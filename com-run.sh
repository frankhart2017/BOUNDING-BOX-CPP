docker build --rm --squash --no-cache -t bounding_box:latest .
docker run -p 8080:8080 -e PORT=8080 -e MONGODB_URI="mongodb://admin:password98@ds131621.mlab.com:31621/heroku_nqv061hq" bounding_box:latest
