#include "bcrypt/bcrypt.h"
#include <jwt-cpp/jwt.h>
#include <iostream>
#include <string>
#include <vector>

class Tools
{
public:

    class Place {
    public:
        std::string name;
        std::string location;
        std::string description;
        float score;
        std::string photos;

        // Updated constructor
        Place(const std::string& name, const std::string& location, const std::string& description, float score)
            : name(name), location(location), description(description), score(score) {}
    };


    std::string urlEncode(const std::string& value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            }
            else {
                escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
            }
        }
        return escaped.str();
    }

    char* bcrypt(const char* message, int workfactor)
    {
        char salt[BCRYPT_HASHSIZE] = { 0 };
        char hash[BCRYPT_HASHSIZE] = { 0 };

        if (bcrypt_gensalt(workfactor, salt) != 0) {
            std::cerr << "Failed to generate salt.\n";
            return NULL;
        }

        //std::cout << "Generated Salt: " << salt << std::endl;

        if (bcrypt_hashpw(message, salt, hash) != 0) {
            std::cerr << "Failed to hash password.\n";
            return NULL;
        }

        //std::cout << "Password Hash: " << hash << std::endl;
        return hash;
    }

    bool verify_password(const char* password, const char* stored_hash) {
        char result[BCRYPT_HASHSIZE] = { 0 };

        // Use bcrypt_hashpw with the stored hash as the salt
        if (bcrypt_hashpw(password, stored_hash, result) != 0) {
            //std::cerr << "Error during password verification.\n";
            return false;
        }

        // Compare the result with the stored hash
        return std::strcmp(result, stored_hash) == 0;
    }

    // Function to generate a JWT token for a user
    std::string generate_jwt(const std::string& username) {
        auto token = jwt::create()
            .set_issuer("auth_server")
            .set_payload_claim("username", jwt::claim(username))
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(1))
            .sign(jwt::algorithm::hs256{ "secret" });
        return token;
    }

    std::vector<Place> parseResponse(const std::string& response) {
        std::vector<Place> places;
        size_t startPos = 0;

        while (true) {
            // Find the start of the next place section
            size_t nameStart = response.find("**", startPos);
            if (nameStart == std::string::npos) break;

            size_t nameEnd = response.find("**", nameStart + 2);
            if (nameEnd == std::string::npos) break;

            // Extract name of the place
            std::string name = response.substr(nameStart + 2, nameEnd - nameStart - 2);

            // Look for location section after the name
            size_t locationStart = response.find("**location**:", nameEnd);
            if (locationStart == std::string::npos) break;

            locationStart += 13; // Skip past "**location**:"
            size_t locationEnd = response.find("\n", locationStart);

            std::string location = response.substr(locationStart, locationEnd - locationStart);

            // Look for description section after the location
            size_t descriptionStart = response.find("**description**:", locationEnd);
            if (descriptionStart == std::string::npos) break;

            descriptionStart += 16; // Skip past "**description**:"
            size_t descriptionEnd = response.find("\n", descriptionStart);

            std::string description = response.substr(descriptionStart, descriptionEnd - descriptionStart);

            // Look for score section after the description
            size_t scoreStart = response.find("**score**:", descriptionEnd);
            if (scoreStart == std::string::npos) break;

            scoreStart += 10; // Skip past "**score**:"
            size_t scoreEnd = response.find("\n", scoreStart);

            std::string scoreStr = response.substr(scoreStart, scoreEnd - scoreStart);
            float score = std::stof(scoreStr); // Convert the score to a float

            // Store parsed information
            places.push_back({ name, location, description, score });

            // Move to the next part after the score
            startPos = scoreEnd;
        }

        return places;
    }

};