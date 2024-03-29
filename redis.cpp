#include <syslog.h>
#include <HalonMTA.h>
#include <sw/redis++/redis++.h>
#include <queue>
#include <chrono>

using namespace sw::redis;

class MyRedis
{
	public:
		std::shared_ptr<Redis> redis;
		std::shared_ptr<RedisCluster> redis_cluster;
};

std::vector<std::pair<std::string, std::shared_ptr<Redis>>> redis;
std::vector<std::pair<std::string, std::shared_ptr<RedisCluster>>> redis_cluster;

std::string default_profile;

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

	const char* dp = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "default_profile"), nullptr);
	default_profile = dp ? dp : "__default";

	auto x = HalonMTA_config_object_get(cfg, "profiles");
	if (x)
	{
		size_t y = 0;
		HalonConfig* z;
		while ((z = HalonMTA_config_array_get(x, y++)))
		{
			std::string type;
			const char* a = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "type"), nullptr);
			type = a ? a : "standalone";

			std::string host;
			const char* b = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "host"), nullptr);
			host = b ? b : "127.0.0.1";

			int port = 6379;
			const char* c = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "port"), nullptr);
			if (c) port = strtoul(c, nullptr, 10);

			std::string user;
			const char* d = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "user"), nullptr);
			if (d) user = d;

			std::string password;
			const char* e = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "password"), nullptr);
			if (e) password = e;

			int pool_size = 1;
			const char* f = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "pool_size"), nullptr);
			if (f) pool_size = strtoul(f, nullptr, 10);

			int connect_timeout = 0;
			const char* g = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "connect_timeout"), nullptr);
			if (g) connect_timeout = strtoul(g, nullptr, 10);

			int socket_timeout = 0;
			const char* h = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "socket_timeout"), nullptr);
			if (h) socket_timeout = strtoul(h, nullptr, 10);

			int wait_timeout = 0;
			const char* q = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "wait_timeout"), nullptr);
			if (q) wait_timeout = strtoul(q, nullptr, 10);

			int connection_lifetime = 0;
			const char* r = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "connection_lifetime"), nullptr);
			if (r) connection_lifetime = strtoul(r, nullptr, 10);

			int connection_idle_time = 0;
			const char* s = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "connection_idle_time"), nullptr);
			if (s) connection_idle_time = strtoul(s, nullptr, 10);

			bool keep_alive = false;
			const char* t = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "keep_alive"), nullptr);
			if (t) keep_alive = (strcmp(t, "true") == 0 || strcmp(t, "TRUE") == 0) ? true : false;

			int db = 0;
			const char* u = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "db"), nullptr);
			if (u) db = strtoul(u, nullptr, 10);

			std::string master_name;
			const char* i = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "master_name"), nullptr);
			if (i) master_name = i;

			std::vector<std::pair<std::string, int>> hosts;
			auto j = HalonMTA_config_object_get(z, "hosts");
			if (j)
			{
				size_t k = 0;
				HalonConfig* l;
				while ((l = HalonMTA_config_array_get(j, k++)))
				{
					std::string _host;
					const char* m = HalonMTA_config_string_get(HalonMTA_config_object_get(l, "host"), nullptr);
					_host = m ? m : "127.0.0.1";
					int _port;
					const char* n = HalonMTA_config_string_get(HalonMTA_config_object_get(l, "port"), nullptr);
					_port = n ? strtoul(n, nullptr, 10) : type == "sentinel" ? 26379 : 6379;
					hosts.push_back(std::make_pair(_host, _port));
				}
			}
			if (type == "cluster" && hosts.empty()) hosts.push_back(std::make_pair(host, port)); // Backward compatibility

			std::string id;
			const char* o = HalonMTA_config_string_get(HalonMTA_config_object_get(z, "id"), nullptr);
			id = o;

			ConnectionPoolOptions pool_options;
			if (pool_size) pool_options.size = pool_size;
			if (wait_timeout) pool_options.wait_timeout = std::chrono::milliseconds(wait_timeout);
			if (connection_lifetime) pool_options.connection_lifetime = std::chrono::milliseconds(connection_lifetime);
			if (connection_idle_time) pool_options.connection_idle_time = std::chrono::milliseconds(connection_idle_time);

			try {
				if (type == "cluster") {
					for (size_t p = 0; p < hosts.size(); ++p)
					{
						try {
							auto _host = hosts[p];
							ConnectionOptions connection_options;
							if (!_host.first.empty()) connection_options.host = _host.first;
							if (_host.second) connection_options.port = _host.second;
							if (!user.empty()) connection_options.user = user;
							if (!password.empty()) connection_options.password = password;
							if (connect_timeout) connection_options.connect_timeout = std::chrono::milliseconds(connect_timeout);
							if (socket_timeout) connection_options.socket_timeout = std::chrono::milliseconds(socket_timeout);
							if (keep_alive) connection_options.keep_alive = keep_alive;
							if (db) connection_options.db = db;
							redis_cluster.push_back(std::make_pair(id, std::make_shared<RedisCluster>(connection_options, pool_options)));
							break;
						} catch (const Error &err) {
							if (p == (hosts.size() - 1)) {
								syslog(LOG_CRIT, "redis: %s", err.what());
								return false;
							}
						}
					}
				} else if (type == "sentinel") {
					ConnectionOptions connection_options;
					if (!user.empty()) connection_options.user = user;
					if (!password.empty()) connection_options.password = password;
					if (connect_timeout) connection_options.connect_timeout = std::chrono::milliseconds(connect_timeout);
					if (socket_timeout) connection_options.socket_timeout = std::chrono::milliseconds(socket_timeout);
					if (keep_alive) connection_options.keep_alive = keep_alive;
					if (db) connection_options.db = db;
					SentinelOptions sentinel_options;
					sentinel_options.nodes = hosts;
					auto sentinel = std::make_shared<Sentinel>(sentinel_options);
					redis.push_back(std::make_pair(id, std::make_shared<Redis>(sentinel, master_name, Role::MASTER, connection_options, pool_options)));
				} else {
					ConnectionOptions connection_options;
					if (!host.empty()) connection_options.host = host;
					if (port) connection_options.port = port;
					if (!user.empty()) connection_options.user = user;
					if (!password.empty()) connection_options.password = password;
					if (connect_timeout) connection_options.connect_timeout = std::chrono::milliseconds(connect_timeout);
					if (socket_timeout) connection_options.socket_timeout = std::chrono::milliseconds(socket_timeout);
					if (keep_alive) connection_options.keep_alive = keep_alive;
					if (db) connection_options.db = db;
					redis.push_back(std::make_pair(id, std::make_shared<Redis>(connection_options, pool_options)));
				}
			} catch (const Error &err) {
				syslog(LOG_CRIT, "redis: %s", err.what());
				return false;
			}
		}
	}
	else
	{
		try {
			ConnectionOptions connection_options;
			connection_options.host = "127.0.0.1";
			redis.push_back(std::make_pair(default_profile, std::make_shared<Redis>(connection_options)));
		} catch (const Error &err) {
			syslog(LOG_CRIT, "redis: %s", err.what());
			return false;
		}
	}

	return true;
}

void MyRedis_free(void* ptr)
{
	MyRedis* x = (MyRedis*)ptr;
	delete x;
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
			char* av = nullptr;
			size_t al;
			HalonMTA_hsl_value_get(a_, HALONMTA_HSL_TYPE_STRING, &av, &al);
			argv.push_back(av);
			lens.push_back(al);
		}

		MyRedis* ptr = (MyRedis*)HalonMTA_hsl_object_ptr_get(hhc);

		sw::redis::ReplyUPtr result;
		if (ptr->redis_cluster != nullptr) {
			result = ptr->redis_cluster->command(argv.begin(), argv.end());
		} else {
			result = ptr->redis->command(argv.begin(), argv.end());
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
	size_t al;
	char* a = nullptr;
	std::string profile;
	HalonHSLValue* b = HalonMTA_hsl_argument_get(args, 0);
	if (b)
	{
		if (HalonMTA_hsl_value_type(b) != HALONMTA_HSL_TYPE_STRING)
		{
			HalonHSLValue* c = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(c, HALONMTA_HSL_TYPE_EXCEPTION, "argument is not a string", 0);
			return;
		}
		HalonMTA_hsl_value_get(b, HALONMTA_HSL_TYPE_STRING, &a, &al);
	}
	profile = a ? a : default_profile;

	std::shared_ptr<sw::redis::Redis> r;
	for (auto p : redis)
	{
		if (p.first == profile)
		{
			r = p.second;
			break;
		}
	}

	std::shared_ptr<sw::redis::RedisCluster> rc;
	if (!r)
	{
		for (auto p : redis_cluster)
		{
			if (p.first == profile)
			{
				rc = p.second;
				break;
			}
		}
	}

	if (!r && !rc)
	{
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "invalid profile", 0);
		return;
	}

	auto ptr = new MyRedis();
	ptr->redis = r;
	ptr->redis_cluster = rc;

	HalonHSLObject* object = HalonMTA_hsl_object_new();
	HalonMTA_hsl_object_type_set(object, "Redis");
	HalonMTA_hsl_object_register_function(object, "command", HSLRedis_command);
	HalonMTA_hsl_object_ptr_set(object, ptr, MyRedis_free);
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_OBJECT, object, 0);
	HalonMTA_hsl_object_delete(object);
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* hhrc)
{
	HalonMTA_hsl_module_register_function(hhrc, "Redis", HSLRedis);
	return true;
}
