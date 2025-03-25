#include "API.hpp"

int main()
{
    Tools tool;
    API_Travel api_travel;
    crow::SimpleApp app;

    sql::mysql::MySQL_Driver* MySQL_Driver = sql::mysql::get_mysql_driver_instance();
    std::unique_ptr<sql::Connection> MySQL_Connection(MySQL_Driver->connect("tcp://127.0.0.1:3306", "root", "root123"));
    MySQL_Connection->setSchema("travelapp");

    CROW_ROUTE(app, "/api/v1/signup").methods(crow::HTTPMethod::Post)([&api_travel, connection = MySQL_Connection.get()](const crow::request& req) {
        auto body = crow::json::load(req.body);

        // Validate JSON body
        if (!body || body["username"].t() != crow::json::type::String || body["password"].t() != crow::json::type::String || body["gmail"].t() != crow::json::type::String) {
            return crow::response(400, "Invalid JSON format or missing fields");
        }


        if (api_travel.SignUp(body, connection))
        {
            crow::json::wvalue response;
            response["message"] = "SignUp succeeded!";
            response["username"] = body["username"].s();
            response["gmail"] = body["gmail"].s();
            return crow::response(200, response);
        }
        return crow::response(500, "Invalid info, or user already exist!");
        });

    CROW_ROUTE(app, "/api/v1/login").methods(crow::HTTPMethod::Post)([&api_travel, connection = MySQL_Connection.get()](const crow::request& req) {
        auto body = crow::json::load(req.body);

        // Validate JSON body
        if (!body || body["password"].t() != crow::json::type::String || body["gmail"].t() != crow::json::type::String) {
            return crow::response(400, "Invalid JSON format or missing fields");
        }

        std::string username;

        if (api_travel.Login(body,connection,username))
        {
            crow::json::wvalue response;
            response["message"] = "Login succeeded!";
            response["username"] = username;
            response["gmail"] = body["gmail"].s();
            return crow::response(200, response);
        }
        else
        {
            return crow::response(401, "Failed to login, incorrect username or password.");
        }
        });

    CROW_ROUTE(app, "/api/v1/change-email").methods(crow::HTTPMethod::Post)([&api_travel, connection = MySQL_Connection.get()](const crow::request& req) {
        auto body = crow::json::load(req.body);

        // Validate JSON body
        if (!body || body["old_email"].t() != crow::json::type::String || body["new_email"].t() != crow::json::type::String || body["password"].t() != crow::json::type::String) {
            return crow::response(400, "Invalid JSON format or missing fields");
        }

        std::string old_email = body["old_email"].s();
        std::string new_email = body["new_email"].s();
        std::string password = body["password"].s();

        if (!api_travel.authenticateUser(old_email, password, connection)) {
            return crow::response(401, "Unauthorized: Invalid credentials");
        }

        if (api_travel.isEmailTaken(new_email, connection)) {
            return crow::response(400, "Email is already taken");
        }

        bool success = api_travel.updateEmailInDatabase(old_email, new_email, connection);
        if (success) {
            return crow::response(200, "Email successfully changed");
        }
        else {
            return crow::response(500, "Error updating email");
        }
        });

    CROW_ROUTE(app, "/api/v1/SearchPlaces").methods(crow::HTTPMethod::Post)([&api_travel,&tool](const crow::request& req) { // i mostylu didnt optimized it so you need to do it da, or mb i am latter idk
        curl_global_init(CURL_GLOBAL_DEFAULT);
        auto body = crow::json::load(req.body);

        // Validate JSON body
        if (!body || body["query"].t() != crow::json::type::String) {
            return crow::response(400, "Invalid JSON format or missing fields");
        }

        std::string query = body["query"].s();
        std::string apikeyforpixel = "your key";

        std::string AIresponse = api_travel.send_ollama_request(query);

        std::vector<Tools::Place> places = tool.parseResponse(AIresponse);
        if(places.size() == 0)
            AIresponse = api_travel.send_ollama_request(query);

        if(places.size() == 0)
            return crow::response(501, "Something went wrong, please try again!");

        struct curl_slist* headers = nullptr;

        headers = curl_slist_append(headers, ("Authorization: " + apikeyforpixel).c_str());

        for (int i = 0; i < places.size(); i++) {
            std::string response, headerData;

            //std::cout << "Name: " << places[i].name << std::endl;
           // std::cout << "Location: " << places[i].location << std::endl;
           // std::cout << "Description: " << places[i].description << std::endl;
           // std::cout << std::endl;

            // Construct the Pexels API request
            std::string url = "https://api.pexels.com/v1/search?query=" + tool.urlEncode(places[i].name) + "&per_page=2";

            // Send the request
            bool result = api_travel.CurlReq(url, "", headers, response, headerData, "GET");

            try {
                auto jsonResponse = crow::json::load(response);

                if (jsonResponse["photos"].t() == crow::json::type::List) {
                    // Use a vector to collect URLs
                    // Process the photos and extract the "src" -> "original" URL
                    std::vector<crow::json::wvalue> photoUrls;
                    for (const auto& photo : jsonResponse["photos"]) {
                        if (photo["src"].t() == crow::json::type::Object) {
                            // Extract the "original" image URL
                            const auto& originalUrl = photo["src"]["original"];
                            if (originalUrl.t() == crow::json::type::String) {
                                photoUrls.push_back(crow::json::wvalue(originalUrl.s()));
                            }
                        }
                    }


                    // Convert the vector to a JSON list
                    crow::json::wvalue photoList = crow::json::wvalue::list(photoUrls.begin(), photoUrls.end());

                    // Serialize the photo list to a string
                    std::string serializedPhotoList = photoList.dump();

                    // Store the serialized list in places[i].photos
                    places[i].photos = serializedPhotoList;

                    //std::cout << "Extracted photo URLs: " << serializedPhotoList << std::endl;
                }
                else {
                    return crow::response(501, "The 'photos' field is missing or not a valid array in the external API response.");
                }
            }
            catch (const std::exception& e) {
                return crow::response(502, std::string("Error parsing JSON: ") + e.what());
            }


        }
        curl_slist_free_all(headers);
        curl_global_cleanup();

        // Create the JSON response for all places using crow::json::wvalue
        std::vector<crow::json::wvalue> placesArray;  // Use a vector to store places

        // Add each place to the vector
        for (const auto& place : places) {
            crow::json::wvalue placeData;
            placeData["name"] = place.name;
            placeData["location"] = place.location;
            placeData["description"] = place.description;
            placeData["rating"] = place.score;

            // Parse the photos string back into a JSON array
            try {
                auto photoUrls = crow::json::load(place.photos);  // Parse the JSON string
                if (photoUrls && photoUrls.t() == crow::json::type::List) {
                    placeData["photos"] = photoUrls;  // Add as JSON array
                }
                else {
                    placeData["photos"] = crow::json::wvalue(crow::json::type::List);  // Default to empty array
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing photos JSON: " << e.what() << std::endl;
                placeData["photos"] = crow::json::wvalue(crow::json::type::List);  // Default to empty array
            }

            placesArray.push_back(std::move(placeData));  // Add to the array
        }

        // Create the final response
        crow::json::wvalue finalResponse;
        finalResponse["places"] = crow::json::wvalue::list(placesArray.begin(), placesArray.end());

        // Return the response as JSON
        return crow::response(200, finalResponse);
        });

    app.bindaddr("0.0.0.0").port(8080).multithreaded().run();
}

