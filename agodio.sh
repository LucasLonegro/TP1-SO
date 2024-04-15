#!/bin/bash

docker run -ti --rm -v "${PWD}:/app" --privileged agodio/itba-so:2.0 /bin/sh -c "cd /app; exec /bin/bash"
