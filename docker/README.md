Dockerfiles for the Confluo project

### Build docker image
*Run this in root confluo directory*
```
docker build -f docker/Dockerfile.alpine -t confluo:alpine .
```

### Run image
```
docker run -p 9090:9090 -v <localdir>:/var/db confluo:alpine
```
