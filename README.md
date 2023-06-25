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
      type: standalone
      host: redis-node-1
      port: 6379
      user: user
      password: password
      pool_size: 32
      connect_timeout: 5000
      socket_timeout: 5000
```

### Cluster

**smtpd.yaml**

```
plugins:
  - id: redis
    config:
      type: cluster
      host: redis-cluster-node-1
      port: 6379
      user: user
      password: password
      pool_size: 32
      connect_timeout: 5000
      socket_timeout: 5000
```

### Sentinel

**smtpd.yaml**

```
plugins:
  - id: redis
    config:
      type: sentinel
      master_name: redis-sentinel
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
```

## Exported classes

These classes needs to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://redis` module path.

### Redis()

The Redis class is a [redis-plus-plus](https://github.com/sewenew/redis-plus-plus) wrapper class.

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

Values are converted to HSL types, types defined in RESP2 and partly the RESP3 format is supported.

```
echo $redis->command("SET", "key", "value"); // OK
```
