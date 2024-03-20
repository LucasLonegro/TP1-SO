# docker run --rm -v "${PWD}:/app" --privileged agodio/itba-so:2.0 /bin/make -C /app [command]
docker run --rm -v "${PWD}:/app" --privileged agodio/itba-so:2.0 /bin/make -C /app all
# docker run --rm -v "${PWD}:/app" --privileged agodio/itba-so:2.0 /bin/make -C /app valgrind
# docker run --rm -v "${PWD}:/app" --privileged agodio/itba-so:2.0 /bin/make -C /app pvs
