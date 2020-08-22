#!/bin/bash
GIT_PATH=/home/stt/prod/cpp-image-analysis
SITE_PATH=/home/stt/prod/website

pushd $GIT_PATH

git pull 2>&1
if [ $? -ne 0 ]
then
    echo "Failed during git pull"
    exit 1
fi

# TODO: versioning?
docker build --tag stt-datacore/cpp-image-analysis:latest .
if [ $? -ne 0 ]
then
    echo "Failed during Docker build"
    exit 3
fi

popd

# TODO: remove old image and restart; is there a best practices for this?
docker stop DCImageAnalysis
docker rm DCImageAnalysis

# See https://docs.docker.com/engine/reference/commandline/run/#set-environment-variables--e---env---env-file for env.list

docker run -d --name=DCImageAnalysis \
    --restart unless-stopped \
    --publish 5001:5001 \
    --mount type=bind,source="$SITE_PATH",target=/sitedata \
    --mount type=bind,source="$GIT_PATH",target=/traindata \
    stt-datacore/cpp-image-analysis:latest \
    --asseturl=https://assets.datacore.app/ \
    --trainpath=/traindata/ \
    --jsonpath=/sitedata/static/structured/

