# Redis client plugin

This plugin is a wrapper around [redis-plus-plus](https://github.com/sewenew/redis-plus-plus).

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-redis
```

### RHEL

```
yum install halon-extras-redis
```

## Configuration

For the configuration schema, see [redis.schema.json](redis.schema.json).

### Standalone

**smtpd.yaml**

```
plugins:
  - id: redis
    config:
      default_profile: standalone-1
      profiles:
        - id: standalone-1
          type: standalone
          host: redis-node-1
          port: 6379
          user: user
          password: password
          pool_size: 32
          connect_timeout: 5000
          socket_timeout: 5000
          wait_timeout: 5000
          connection_lifetime: 600000
          connection_idle_time: 5000
          keep_alive: true
          db: 0
          resp: 2
```

### Cluster

**smtpd.yaml**

```
plugins:
  - id: redis
    config:
      default_profile: cluster-1
      profiles:
        - id: cluster-1
          type: cluster
          hosts:
            - host: redis-cluster-node-1
              port: 6379
            - host: redis-cluster-node-2
              port: 6379
            - host: redis-cluster-node-3
              port: 6379
          user: user
          password: password
          pool_size: 32
          connect_timeout: 5000
          socket_timeout: 5000
          wait_timeout: 5000
          connection_lifetime: 600000
          connection_idle_time: 5000
          keep_alive: true
          db: 0
          resp: 2
```

### Sentinel

**smtpd.yaml**

```
plugins:
  - id: redis
    config:
      default_profile: sentinel-1
      profiles:
        - id: sentinel-1
          type: sentinel
          master_name: redis-sentinel
          sentinel_user: user
          sentinel_password: password
          hosts:
            - host: redis-sentinel-sentinel-1
              port: 26379
            - host: redis-sentinel-sentinel-2
              port: 26379
            - host: redis-sentinel-sentinel-3
              port: 26379
          user: user
          password: password
          pool_size: 32
          connect_timeout: 5000
          socket_timeout: 5000
          wait_timeout: 5000
          connection_lifetime: 5000
          connection_idle_time: 5000
          keep_alive: true
          db: 0
          resp: 2
```

The `user` and `password` options are used for the master, and `sentinel_user` and `sentinel_password` for the sentinels. The `connect_timeout` and `socket_timeout` options apply to both, and default to 1000 ms for sentinel profiles (they cannot be 0). Blocking commands such as `BLPOP` that wait longer than `socket_timeout` will time out.

The master is only looked up again through the sentinels when a connection is created. During a failover where the old master is still running, pooled connections keep writing to it until the sentinels demote it, and those writes are lost. Set `connection_lifetime` to a low value, such as 5000 ms, to limit how long that can last.

## Exported classes

These classes needs to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://redis` module path.

### Redis([profile])

The Redis class is a [redis-plus-plus](https://github.com/sewenew/redis-plus-plus) wrapper class.

**Params**

- profile `string` - The config profile

**Returns**: class object

```
import { Redis } from "extras://redis";

$redis = Redis();
echo $redis->command("SET", "key", "value"); // OK
echo $redis->command("GET", "key"); // value
```

#### command(...)

Send a generic command.

**Returns**:

Values are converted to HSL types, types defined in RESP2 and partly the RESP3 format is supported. The protocol version is set per profile using the `resp` option (default `2`). With `resp: 3`, replies such as `HGETALL` are returned as maps (keyed arrays) instead of flat lists.

An error reply throws an exception. Errors nested inside an array, such as a failed command in an `EXEC` result or `redis.error_reply()` in a table returned by a Lua script, are returned as strings instead (e.g. `[1, "ERR boom"]`).

Each call uses a connection from the profile's pool, so commands that change connection state, such as `HELLO`, `SELECT`, `MULTI`/`EXEC` and `WATCH`, only affect the connection that call happened to use, and later calls may use another one. Use the `db` and `resp` options instead of `SELECT` and `HELLO`, and Lua scripts (`EVAL`) instead of transactions (not with cluster profiles, see below).

```
echo $redis->command("SET", "key", "value"); // OK
```

##### Cluster

With a cluster profile, `command()` uses the argument after the command name as the key that decides which node receives the command. This means that:

- At least two arguments are required, so commands without arguments such as `PING` throw.
- Commands whose first argument isn't a key, such as `EVAL`, `EVALSHA`, `FCALL`, `XREAD`, `XREADGROUP`, `BITOP`, `LMPOP`, `ZMPOP`, `OBJECT` and `MEMORY`, are routed by that argument instead (for `EVAL`, the script). They only work when it happens to hash to the same node as the key, and otherwise throw `Failed to send command with key: ...`, so don't use them with cluster profiles.
- Commands without keys, such as `INFO server`, are sent to a single node, chosen by the hash of their first argument.
- As always with Redis Cluster, keys used together in one command must be in the same hash slot (use hash tags such as `{user1}:name` and `{user1}:age`).
