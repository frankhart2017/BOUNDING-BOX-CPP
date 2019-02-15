#include "crow_all.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <glob.h>
#include <boost/filesystem.hpp>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

using mongocxx::cursor;

using namespace std;
using namespace crow;
using namespace crow::mustache;

struct path_leaf_string
{
    std::string operator()(const boost::filesystem::directory_entry& entry) const
    {
        return entry.path().leaf().string();
    }
};

void read_directory(const std::string& name, vector<string>& v)
{
    boost::filesystem::path p(name);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(v), path_leaf_string());
}

void getView(response &res, const string &filename, context &x) {
  res.set_header("Content-Type", "text/html");
  auto text = load("public/" + filename + ".html").render(x);
  res.write(text);
  res.end();
}

void sendFile(response &res, string filename, string contentType) {
  ifstream in("public/" + filename, ifstream::in);
  if(in) {
    ostringstream contents;
    contents << in.rdbuf();
    in.close();
    res.set_header("Content-Type", contentType);
    res.write(contents.str());
  } else {
    res.code = 404;
    res.write("Not found");
  }
  res.end();
}

void sendHtml(response &res, string filename) {
  sendFile(res, filename + ".html", "text/html");
}

void sendImage(response &res, string filename) {
  sendFile(res, "images/" + filename, "image/*");
}

void sendScript(response &res, string filename) {
  sendFile(res, "scripts/" + filename, "text/javascript");
}

void sendStyle(response &res, string filename) {
  sendFile(res, "styles/" + filename, "text/css");
}

void notFound(response &res, const string &message) {
  res.code = 404;
  res.write(message + ": Not Found");
  res.end();
}

int read_index(mongocxx::collection collection) {

  mongocxx::cursor cursor = collection.find(document{} << "id" << 1 << finalize);
  int val;
  for(auto doc : cursor) {
      val = doc["i"].get_int32();
  }

  return val;

}

void update_index(mongocxx::collection collection, int val) {
  val++;
  collection.update_one(document{} << "id" << 1 << finalize,
                      document{} << "$set" << open_document <<
                        "i" << val << close_document << finalize);
}

int main(int argc, char *argv[]) {

  crow::SimpleApp app;
  set_base(".");

  mongocxx::instance inst{};

  string mongoConnect = std::string(getenv("MONGODB_URI"));

  mongocxx::client conn{mongocxx::uri{mongoConnect}};

  auto index = conn["heroku_nqv061hq"]["index"];
  auto image = conn["heroku_nqv061hq"]["images"];

  vector<string> v;
  read_directory("public/images/", v);

  CROW_ROUTE(app, "/styles/<string>")
    ([](const request &req, response &res, string filename){
      sendStyle(res, filename);
    });

  CROW_ROUTE(app, "/scripts/<string>")
    ([](const request &req, response &res, string filename){
      sendScript(res, filename);
    });
  CROW_ROUTE(app, "/images/<string>")
    ([](const request &req, response &res, string filename){
      sendImage(res, filename);
    });

  CROW_ROUTE(app, "/").methods("GET"_method, "POST"_method)
    ([&index, &v, &image](const request &req, response &res){

      if(req.method == "GET"_method) {
        crow::mustache::context ctx;

        int cur_idx = read_index(index);

        string file = v[cur_idx];
        ctx["image"] = "images/" + file;

        getView(res, "index", ctx);
      }

    });

  CROW_ROUTE(app, "/image").methods("POST"_method)
    ([&index, &v, &image](const request &req, response &res){

      if(req.method == "POST"_method) {
        crow::mustache::context ctx;

        int cur_idx = read_index(index);

        string file = v[cur_idx];
        ctx["image"] = "images/" + file;

        auto data = crow::json::load(req.body);

        ostringstream x1_str, y1_str, x2_str, y2_str, otype_str;
        x1_str<<data["x1"];
        y1_str<<data["y1"];
        x2_str<<data["x2"];
        y2_str<<data["y2"];
        otype_str<<data["otype"];

        string x1, y1, x2, y2, otype;
        x1 = x1_str.str();
        y1 = y1_str.str();
        x2 = x2_str.str();
        y2 = y2_str.str();
        otype = otype_str.str();

        boost::replace_all(otype, "\"", "");

        auto builder = bsoncxx::builder::stream::document{};
        bsoncxx::document::value doc = builder
          << "image" << v[cur_idx]
          << "otype" << otype
          << "x1" << x1
          << "y1" << y1
          << "x2" << x2
          << "y2" << y2
          << bsoncxx::builder::stream::finalize;

        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = image.insert_one(doc.view());

        update_index(index, cur_idx);

        getView(res, "index", ctx);
      }

    });

  char* port = getenv("PORT");
  uint16_t iPort = static_cast<uint16_t>(port != NULL? stoi(port): 18080);
  cout<<"PORT = "<<iPort<<"\n";

  app.port(iPort).multithreaded().run();

  return 0;
}
