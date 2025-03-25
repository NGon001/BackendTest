#include "crow_all.h"
#include "Tools.hpp"


#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>

#include <nlohmann/json.hpp>

#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#define CURL_STATICLIB
#include <curl/curl.h>

class API_Travel {
public:
	Tools tool;

    // Implementations are included within the class definition
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        //LOG("WriteCallback");
        size_t total_size = size * nmemb;
        output->append(reinterpret_cast<char*>(contents), total_size);
        return total_size;
    }

    static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        //LOG("HeaderCallback");
        std::string header((char*)contents, size * nmemb);
        std::string* headerData = static_cast<std::string*>(userp);

        // Check for X-SECURITY-TOKEN
        if (header.find("X-SECURITY-TOKEN:") == 0) {
            size_t pos = header.find(":");
            if (pos != std::string::npos) {
                std::string token = header.substr(pos + 1);
                token.erase(std::remove(token.begin(), token.end(), ' '), token.end()); // Trim whitespace
                *headerData += "X-SECURITY-TOKEN:" + token + "\n";
                return size * nmemb;
            }
        }

        // Check for CST
        if (header.find("CST:") == 0) {
            size_t pos = header.find(":");
            if (pos != std::string::npos) {
                std::string token = header.substr(pos + 1);
                token.erase(std::remove(token.begin(), token.end(), ' '), token.end()); // Trim whitespace
                *headerData += "CST:" + token + "\n";
                return size * nmemb;
            }
        }

        return size * nmemb;
    }

    bool CurlIn(CURL*& curl)
    {
        CURLcode res;

        curl = curl_easy_init();
        if (!curl)
            return 0;
        return 1;
    }

    bool CurlClean(CURL*& curl, struct curl_slist*& headers)
    {
        curl_easy_cleanup(curl);
        return 1;
    }

    bool CurlReq(const std::string& url, const std::string& postFields, curl_slist* headers, std::string& response, std::string& headerData, const std::string& requestType) {
        CURL* curl;
        if (!CurlIn(curl)) return 0;

        if (curl == nullptr) {
            std::cerr << "Invalid CURL handle" << std::endl;
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        if (headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);

        if (requestType == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        }
        else if (requestType == "GET") {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }
        else if (requestType == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        else if (requestType == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        }
        else {
            std::cerr << "Unsupported request type: " << requestType << std::endl;
            CurlClean(curl, headers);
            return false;
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            CurlClean(curl, headers);
            return false;
        }

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        //  std::cout << "HTTP Response Code: " << response_code << std::endl;

        CurlClean(curl, headers);
        return true;
    }

	bool isEmailTaken(const std::string& new_email, sql::Connection* connection) {
		std::unique_ptr<sql::PreparedStatement> pstmt(connection->prepareStatement("SELECT id FROM users WHERE gmail = ?"));
		pstmt->setString(1, new_email);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		return res->next();  // If the query returns a row, the email is already taken
	}

	bool updateEmailInDatabase(const std::string& old_email, const std::string& new_email, sql::Connection* connection) {
		std::unique_ptr<sql::PreparedStatement> pstmt(connection->prepareStatement("UPDATE users SET gmail = ? WHERE gmail = ?"));
		pstmt->setString(1, new_email);
		pstmt->setString(2, old_email);
		return pstmt->executeUpdate() > 0;
	}

	bool authenticateUser(const std::string& old_email, const std::string& password, sql::Connection* connection) {
		std::unique_ptr<sql::PreparedStatement> pstmt(connection->prepareStatement("SELECT password FROM users WHERE gmail = ?"));
		pstmt->setString(1, old_email);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			std::string stored_password = res->getString("password");
			return true;
		}
		return false;
	}

	bool SignUp(crow::json::rvalue body, sql::Connection* MySQL_Connection)
	{
		// Extract body
		std::string username = body["username"].s();
		std::string password = body["password"].s();
		char* hash = tool.bcrypt(password.c_str(),12);
		std::string gmail = body["gmail"].s();
		std::string token = tool.generate_jwt(username);

		std::unique_ptr<sql::PreparedStatement> pstmt(MySQL_Connection->prepareStatement("INSERT INTO users (gmail, username, password,token) VALUES (?, ?, ?,?)"));
		pstmt->setString(1, gmail);
		pstmt->setString(2, username);
		pstmt->setString(3, hash);
		pstmt->setString(4, token);

		// Execute the statement
		pstmt->executeUpdate();


		std::cout << "User " << username << " stored successfully!" << std::endl;
		return true;
	}

    bool Login(crow::json::rvalue body, sql::Connection* MySQL_Connection,std::string& username)
    {
        std::string gmail = body["gmail"].s();
        std::string password = body["password"].s();
        std::string token = tool.generate_jwt(gmail);

        // Prepare a statement to find the user
        std::unique_ptr<sql::PreparedStatement> pstmt(
            MySQL_Connection->prepareStatement("SELECT password, username FROM users WHERE gmail = ?"));

        // Bind the username to the query
        pstmt->setString(1, gmail);

        // Execute the query
        std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

        // Check if the user exists
        if (!result->next()) {
            std::cout << "Invalid username or password!" << std::endl;
            return false;
        }

        std::string stored_hash = result->getString("password");
        username = result->getString("username");

        if (tool.verify_password(password.c_str(), stored_hash.c_str()))
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(MySQL_Connection->prepareStatement("INSERT INTO users (token) VALUES (?)"));
            pstmt->setString(1, token);
            return true;
        }
        else
        {
            return false;
        }
    }


    // Function to parse and extract responses from multiple JSON objects in the response
    std::string process_json_responses(const std::string& raw_response) {
        std::istringstream stream(raw_response);
        std::string line;
        std::string full_response;

        // Process each line as a separate JSON object
        while (std::getline(stream, line)) {
            try {
                // Parse the current line as a JSON object
                auto json_response = nlohmann::json::parse(line);

                // If the "response" field exists, concatenate it to the full response
                if (json_response.contains("response")) {
                    full_response += json_response["response"].get<std::string>();
                }
            }
            catch (const nlohmann::json::parse_error& e) {
                std::cerr << "Error parsing JSON in line: " << e.what() << std::endl;
            }
        }
        return full_response;
    }

    std::string send_ollama_request(std::string place) {

        // URL of the Ollama local server
        std::string endpoint = "http://localhost:11434/api/generate";

        std::string response;
        std::string headerData;

        struct curl_slist* headers = nullptr;

        headers = curl_slist_append(headers, "Content-Type: application/json");

        nlohmann::json payload = {
           {"model", "llama3.2"}, // Specify Llama 3.2 as the model
     {"prompt", "Please provide a list of places to visit in " + place + ". Make sure to give the full name of the place, the specific location, and a detailed description. For each place, format the response as follows:\n\n1) **place**: [Name of the place]\n   **location**: [Detailed location with address or nearby landmarks]\n   **description**: [Description of the place]\n   **score**: [Score from 0 to 5]\n\nEnsure that the names of the places are specific, like including the full name of the place and its exact location, not just general terms like 'place'."}
        };

        std::string body = payload.dump();

        bool success = CurlReq(endpoint, body, headers, response, headerData, "POST");

        return process_json_responses(response);
    }

};




