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

Each profile needs a unique `id`, otherwise the plugin fails to start. If `default_profile` is set, it must match one of the profiles, otherwise the plugin fails to start; if it's not set, `Redis()` must be called with a profile. If no `profiles` are configured at all, `Redis()` connects to `127.0.0.1:6379`.

For standalone and cluster profiles, `connect_timeout` and `socket_timeout` default to no timeout: an unreachable server blocks the calling script until the operating system gives up (often minutes), and a server that stops responding blocks it indefinitely. Setting them is recommended. Note that with any profile type, blocking commands such as `BLPOP` that wait longer than `socket_timeout` will time out.

`pool_size` defaults to 1, so all scripts using a profile share a single connection and wait for their turn, and `wait_timeout` defaults to no timeout, so they wait for a free connection indefinitely. Set both to suit your load.

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

Redis Cluster only supports database 0, so `db` can only be set to `0` for cluster profiles.

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

The `user` and `password` options are used for the master, and `sentinel_user` and `sentinel_password` for the sentinels. The `connect_timeout` and `socket_timeout` options apply to both, and default to 1000 ms for sentinel profiles (they cannot be 0).

The master is only looked up again through the sentinels when a connection is created. During a failover where the old master is still running, pooled connections keep writing to it until the sentinels demote it, and those writes are lost. Set `connection_lifetime` to a low value, such as 5000 ms, to limit how long that can last.

## Exported classes

These classes need to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://redis` module path.

### Redis([profile])

Creates a client for a configured profile.

**Params**

- profile `string` - The config profile (optional, defaults to `default_profile`)

**Returns**: class object

Throws an exception if the profile doesn't exist (`invalid profile`) or isn't a string (`argument is not a string`).

```
import { Redis } from "extras://redis";

$redis = Redis();
echo $redis->command("SET", "key", "value"); // OK
echo $redis->command("GET", "key"); // value
```

#### command(...)

Sends a generic command.

**Params**

- ...args `string` - The command name and its arguments. All arguments must be strings, so pass numbers as strings (e.g. `"60"`, not `60`), otherwise an exception is thrown.

**Returns**:

The reply is converted to an HSL value:

| Redis reply | HSL value |
|---|---|
| string, status (e.g. `OK`) | string |
| verbatim string, big number (RESP3) | string |
| error, nested inside an array, set or map | string |
| integer | number |
| double (RESP3) | number |
| nil (e.g. `GET` on a missing key) | none |
| boolean (RESP3) | boolean |
| array | array (list) |
| set (RESP3) | array (list) |
| map (RESP3) | array (keyed) |
| attribute, push (RESP3) | none (unsupported) |

The protocol version is set per profile using the `resp` option (default `2`). With `resp: 3`, replies such as `HGETALL` are returned as maps (keyed arrays) instead of flat lists.

HSL numbers are only exact up to 2^53 (9007199254740992), so larger integer replies are rounded. The value stored in Redis is unaffected; to get it exactly, read it as a string instead (e.g. `GET` rather than `INCRBY key 0`).

An error reply, a connection error or a timeout throws an exception. Errors nested inside an array, such as `redis.error_reply()` in a table returned by a Lua script, are returned as strings instead (e.g. `[1, "ERR boom"]`).

Each call uses a connection from the profile's pool and returns it afterwards. Commands that change connection state, such as `HELLO`, `SELECT`, `MULTI`/`EXEC` and `WATCH`, therefore only affect the connection that call happened to use: your next call may get another connection, and the changed connection is later reused by other calls and scripts. Use the `db` and `resp` options instead of `SELECT` and `HELLO`, and Lua scripts (`EVAL`) instead of transactions (not with cluster profiles, see below).

Don't use `SUBSCRIBE`, `PSUBSCRIBE`, `SSUBSCRIBE` or `MONITOR`. With `resp: 2`, and for `MONITOR` with either version, they leave the connection in a mode where every later command on it fails, for all scripts using the profile; by default this lasts until the process is restarted, and setting `connection_lifetime` limits it. With `resp: 3`, the subscribe commands never get a reply, so the call blocks until `socket_timeout`, or indefinitely if it's not set.

```
echo $redis->command("SET", "key", "value", "EX", "60"); // OK
echo $redis->command("GET", "missing"); // none
```

##### Cluster profiles

With a cluster profile, `command()` uses the argument after the command name as the key that decides which node receives the command. This means that:

- At least two arguments are required, so commands without arguments such as `PING` throw.
- Commands whose first argument isn't a key, such as `EVAL`, `EVALSHA`, `FCALL`, `XREAD`, `XREADGROUP`, `BITOP`, `LMPOP`, `ZMPOP`, `OBJECT` and `MEMORY`, are routed by that argument instead (for `EVAL`, the script). They only work when it happens to hash to the same node as the key, and otherwise throw `Failed to send command with key: ...`, so don't use them with cluster profiles.
- Commands without keys, such as `INFO server`, are sent to a single node, chosen by the hash of their first argument.
- As always with Redis Cluster, keys used together in one command must be in the same hash slot (use hash tags such as `{user1}:name` and `{user1}:age`).
