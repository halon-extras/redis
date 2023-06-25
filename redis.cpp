#include <syslog.h>
#include <HalonMTA.h>
#include <sw/redis++/redis++.h>
#include <queue>
#include <chrono>

using namespace sw::redis;

std::shared_ptr<Redis> redis;
std::shared_ptr<RedisCluster> redis_cluster;

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}

HALON_EXPORT
bool Halon_init(HalonInitContext* hic)
{
	HalonConfig* cfg = nullptr;
	HalonMTA_init_getinfo(hic, HALONMTA_INIT_CONFIG, nullptr, 0, &cfg, nullptr);
	if (!cfg)
		return false;

	std::string type;
	const char* a = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "type"), nullptr);
	type = a ? a : "standalone";

	std::string host;
	const char* b = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "host"), nullptr);
	host = b ? b : "127.0.0.1";

	int port;
	const char* c = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "port"), nullptr);
	port = c ? strtoul(c, nullptr, 10) : 6379;

	std::string user;
	const char* d = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "user"), nullptr);
	if (d) user = d;

	std::string password;
	const char* e = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "password"), nullptr);
	if (e) password = e;

	int pool_size;
	const char* f = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "pool_size"), nullptr);
	pool_size = f ? strtoul(f, nullptr, 10) : 1;

	int connect_timeout;
	const char* g = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "connect_timeout"), nullptr);
	if (g) connect_timeout = strtoul(g, nullptr, 10);

	int socket_timeout;
	const char* h = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "socket_timeout"), nullptr);
	if (h) socket_timeout = strtoul(h, nullptr, 10);

	std::string master_name;
	const char* i = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "master_name"), nullptr);
	if (i) master_name = i;

	std::vector<std::pair<std::string, int>> hosts;
	auto j = HalonMTA_config_object_get(cfg, "hosts");
	if (j)
	{
		size_t k = 0;
		HalonConfig* l;
		while ((l = HalonMTA_config_array_get(j, k++)))
		{
			std::string sentinel_host;
			const char* m = HalonMTA_config_string_get(HalonMTA_config_object_get(l, "host"), nullptr);
			sentinel_host = m ? m : "127.0.0.1";
			int sentinel_port;
			const char* n = HalonMTA_config_string_get(HalonMTA_config_object_get(l, "port"), nullptr);
			sentinel_port = n ? strtoul(n, nullptr, 10) : 26379;
			hosts.push_back(std::make_pair(sentinel_host, sentinel_port));
		}
	}

	ConnectionOptions connection_options;
	if ((type == "standalone" || type == "cluster") && !host.empty()) connection_options.host = host;
	if ((type == "standalone" || type == "cluster") && port) connection_options.port = port;
	if (!user.empty()) connection_options.user = user;
	if (!password.empty()) connection_options.password = password;
	if (connect_timeout) connection_options.connect_timeout = std::chrono::milliseconds(connect_timeout);
	if (socket_timeout) connection_options.socket_timeout = std::chrono::milliseconds(socket_timeout);

	ConnectionPoolOptions pool_options;
	if (pool_size) pool_options.size = pool_size;

	try {
		if (type == "cluster") {
			redis_cluster = std::make_shared<RedisCluster>(connection_options, pool_options);
		} else if (type == "sentinel") {
			SentinelOptions sentinel_options;
			sentinel_options.nodes = hosts;
			auto sentinel = std::make_shared<Sentinel>(sentinel_options);
			redis = std::make_shared<Redis>(sentinel, master_name, Role::MASTER, connection_options, pool_options);
		} else {
			redis = std::make_shared<Redis>(connection_options, pool_options);
		}
	} catch (const Error &err) {
		syslog(LOG_INFO, "redis: %s", err.what());
		return false;
	}

	return true;
}

void HSLRedis_command(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	try {
		std::vector<const char*> argv;
		std::vector<size_t> lens;
		size_t a = 0;
		while (HalonHSLValue* a_ = HalonMTA_hsl_argument_get(args, a++))
		{
			if (HalonMTA_hsl_value_type(a_) != HALONMTA_HSL_TYPE_STRING)
			{
				HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
				HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "argument is not a string", 0);
				return;
			}
			char* a = nullptr;
			size_t al;
			HalonMTA_hsl_value_get(a_, HALONMTA_HSL_TYPE_STRING, &a, &al);
			argv.push_back(a);
			lens.push_back(al);
		}

		sw::redis::ReplyUPtr result;
		if (redis_cluster != nullptr) {
			result = redis_cluster->command(argv.begin(), argv.end());
		} else {
			result = redis->command(argv.begin(), argv.end());
		}
		auto reply = result.get();

		struct type
		{
			HalonHSLValue* v;
			enum {
				PLAIN,
				KEY,
				VALUE
			} type;
			size_t idx;
		};

		std::queue<std::pair<redisReply*, type>> stack;

		stack.push({ reply, { ret, type::PLAIN, 0 } });
		while (!stack.empty())
		{
			redisReply* r = stack.front().first;
			HalonHSLValue* v;
			switch (stack.front().second.type)
			{
				case type::PLAIN:
					v = stack.front().second.v;
				break;
				case type::VALUE:
					v = HalonMTA_hsl_value_array_get(stack.front().second.v, stack.front().second.idx, nullptr);
				break;
				case type::KEY:
					HalonMTA_hsl_value_array_get(stack.front().second.v, stack.front().second.idx, &v);
				break;
			}
			stack.pop();

			/* handle types of RESP2 and RESP3 */
			switch (r->type)
			{
				case REDIS_REPLY_STRING:
				case REDIS_REPLY_STATUS:
				case REDIS_REPLY_ERROR:
				case REDIS_REPLY_BIGNUM:
				{
					HalonMTA_hsl_value_set(v, HALONMTA_HSL_TYPE_STRING, r->str, r->len);
				}
				break;
				case REDIS_REPLY_ARRAY:
				case REDIS_REPLY_SET:
				{
					HalonMTA_hsl_value_set(v, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);
					for (size_t i = 0; i < r->elements; ++i)
					{
						HalonHSLValue *k, *v2;
						HalonMTA_hsl_value_array_add(v, &k, &v2);
						double d = i;
						HalonMTA_hsl_value_set(k, HALONMTA_HSL_TYPE_NUMBER, &d, 0);
						stack.push({ r->element[i], { v, type::VALUE, i }});
					}
				}
				break;
				case REDIS_REPLY_INTEGER:
				{
					double x = r->integer; /* truncated to double */
					HalonMTA_hsl_value_set(v, HALONMTA_HSL_TYPE_NUMBER, (const void*)&x, 0);
				}
				break;
				case REDIS_REPLY_NIL:
					/* null */
				break;
				case REDIS_REPLY_DOUBLE:
					HalonMTA_hsl_value_set(v, HALONMTA_HSL_TYPE_NUMBER, (const void*)&r->dval, 0);
				break;
				case REDIS_REPLY_BOOL:
				{
					bool x = r->integer ? true : false;
					HalonMTA_hsl_value_set(v, HALONMTA_HSL_TYPE_BOOLEAN, (const void*)&x, 0);
				}
				break;
				case REDIS_REPLY_MAP:
				{
					HalonMTA_hsl_value_set(v, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);
					for (size_t i = 0, x = 0; i < r->elements; i += 2, ++x)
					{
						HalonHSLValue *k, *v2;
						HalonMTA_hsl_value_array_add(v, &k, &v2);
						stack.push({ r->element[i], { v, type::KEY, x }});
						stack.push({ r->element[i + 1], { v, type::VALUE, x }});
					}
				}
				break;
				case REDIS_REPLY_ATTR:
					/* unsupported */
				break;
				case REDIS_REPLY_PUSH:
					/* unsupported */
				break;
				case REDIS_REPLY_VERB:
					/* unsupported: string */
				break;
			}
		}
	} catch (const Error &err) {
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
		return;
	}
}

void HSLRedis(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	HalonHSLObject* object = HalonMTA_hsl_object_new();
	HalonMTA_hsl_object_type_set(object, "Redis");
	HalonMTA_hsl_object_register_function(object, "command", HSLRedis_command);
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_OBJECT, object, 0);
	HalonMTA_hsl_object_delete(object);
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* hhrc)
{
	HalonMTA_hsl_module_register_function(hhrc, "Redis", HSLRedis);
	return true;
}
