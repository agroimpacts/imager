# Franklin deployment

This document will outline how to run Franklin on an EC2 instance with an already existing database.
It will rely on a compose file that brings up a container running Franklin and another container running
PostgreSQL with PostGIS. Franklin's database will run in a separate container to avoid possible version
mismatches and the additional infrastructure overhead of resolving those, since I don't know what labeler
runs on.

The goal of this deployment doc is to walk you through:

- bringing up a Franklin container and database container
- verifying connectivity with the running Franklin instance

The assumptions this guide is based on are:

- DNS already exists to access services running on the EC2 instance over the internet
- the existing labeler service is already accessible via https on the standard port (443)

## Bringing up containers

We can start with the `docker-compose` file that's almost identical to the one in the repo.
That looks like this:

```yaml
version: '2.3'
services:
  database:
    image: quay.io/azavea/postgis:3-postgres12.2-slim
    environment:
      - POSTGRES_USER=franklin
      - POSTGRES_PASSWORD=franklin
      - POSTGRES_DB=franklin
    expose:
      - 5432
    healthcheck:
      test: ["CMD", "pg_isready", "-U", "franklin"]
      interval: 3s
      timeout: 3s
      retries: 3
      start_period: 5s
    command: postgres -c log_statement=all
    restart: always
  franklin:
    image: quay.io/azavea/franklin:b83e39c
    depends_on:
      database:
        condition: service_healthy
    command:
      - serve
      - --with-transactions
      - --with-tiles
      - --run-migrations
    volumes:
      - ./:/opt/franklin/
      - $HOME/.aws:/var/lib/franklin/.aws
    environment:
      - ENVIRONMENT=development
      - DB_HOST=database.service.internal
      - DB_NAME=franklin
      - DB_USER=franklin
      - DB_PASSWORD=franklin
      - AWS_PROFILE=default
      - AWS_REGION
    links:
      - database:database.service.internal
    ports:
      - "9090:9090"
    restart: always
```

This compose file will create a Franklin service available on port 9090 and a database service
that is not accessible on the host. The database service being accessible only in the docker
network prevents port conflicts with the existing running database.

You can save it in `$HOME/docker-compose.yml`, then run `docker-compose up -d` from `$HOME`.

The difference between these services and the ones defined in the repository is the presence of
`restart: always`. This should ensure that the services come up each time after rebooting the
ec2 instance.

## Verifying connectivity

You should already have some kind of DNS that routes HTTP traffic to the EC2 instance if you're
able to access the labeler application over the internet. We'll piggy-back on that rather than
dealing up any new records. The first step to verifying connectivity is, from outside the ec2
instance, trying to make a request to the root URL (`<host name>:9090/`), and troubleshooting.

You can do the first step with `curl`:

```
curl https://<hostname>:9090
```

If that works (you'll know it worked if you see a nice JSON response), congratulations, you're
done! If that doesn't, the first debugging step is
to check the [security groups](https://console.aws.amazon.com/vpc/home?region=us-east-1#securityGroups:)
that apply to the EC2 instance. That security group needs to allow inbound TCP connections on the port
that Franklin is running on (9090, unless you change something in the compose file).

If that works, but then after following the notebook steps documented in the
[`stac-client/`](../stac-client) directory, you can't view imagery, then where Franklin is running might
not have permission to read from s3. If the EC2 instance you've deployed into has AWS credentials configured,
you should ensure that the related user has permission to read from s3. If the EC2 instance has no credentials
configured, you can allow access to s3 from the instance by
[creating an IAM instance profile](https://aws.amazon.com/premiumsupport/knowledge-center/ec2-instance-access-s3-bucket/)
with the appropriate permissions.