
# Travel App Backend
This is a C++ backend for a travel application, built using the Crow framework. It provides authentication, email updates, and place search functionality using external APIs.


## Features

- User signup and login with hashed passwords (bcrypt)
- Secure authentication with JWT tokens
- Email update functionality
- Place search using AI (OLAMA) and external image API (Pixels API)


## Libraries and Frameworks Used

- Crow - Lightweight C++ web framework

- MySQL Connector/C++ - Database connection

- nlohmann/json - JSON parsing

- cURL - HTTP requests

- bcrypt - Password hashing

- jwt-cpp - JWT authentication

- Ollama AI - Searching places

- Pixels API - Searching images


## Usage/Examples

- Change it to your data
```c++
std::unique_ptr<sql::Connection> MySQL_Connection(MySQL_Driver->connect("tcp://127.0.0.1:3306", "root", "root123"));

std::string apikeyforpixel = "your key";

app.bindaddr("0.0.0.0").port(8080).multithreaded().run(); 
// If you have a server, change the IP to your server's address.
// Otherwise, keep it as is, and the API will be available at:
// http://localhost:8080/api/v1/signup
```

- To add a new API endpoint in your C++ backend, follow this structure:

```c++
CROW_ROUTE(app, "/api/v1/{your_endpoint}")
    .methods(crow::HTTPMethod::methods)([](const crow::request& req){
        return crow::response(200, "Your response here");
    });

```


- Example code of frontend

```javascript

Future<void> authenticate() async {
    final url = isLogin
        ? 'http://localhost:8080/api/v1/login'
        : 'http://localhost:8080/api/v1/signup';

    final body = isLogin
        ? {
            'gmail': emailController.text, // Login now uses email
            'password': passwordController.text,
          }
        : {
            'username': usernameController.text,
            'password': passwordController.text,
            'gmail': emailController.text, // Sign up includes email
          };

    try {
      final response = await http.post(
        Uri.parse(url),
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode(body),
      );

      if (response.statusCode == 200) {
        Map<String, dynamic> data = jsonDecode(response.body);
        String username = data['username'];

        Navigator.pushReplacement(
          context,
          MaterialPageRoute(
            builder: (context) => MainPage(username: username, email: emailController.text), // Pass email here
          ),
        );
      } else {
        // Display an error dialog if the server responds with an error
        _showErrorDialog('Authentication failed. Please try again.');
      }
    } on SocketException {
      // Catching the SocketException (no network connection or remote refused)
      _showErrorDialog('Network error: Unable to connect to the server. Please check your internet connection.');
    } catch (e) {
      // Catch any other exceptions
      _showErrorDialog('An unexpected error occurred: $e');
    }
  }


```
## API Reference

#### User Signup

```http
POST /api/v1/signup
```

| Parameter | Type     | Description                |
| :-------- | :------- | :------------------------- |
| `email` | `string` | Required. User email |
| `password` | `string` | Required. User password |



#### User login

```http
POST /api/v1/login
```

| Parameter | Type     | Description                       |
| :-------- | :------- | :-------------------------------- |
| `email` | `string` | Required. User email |
| `password` | `string` | Required. User password |

#### Place Search

```http
POST /api/v1/SearchPlaces
```

| Parameter | Type     | Description                       |
| :-------- | :------- | :-------------------------------- |
| `query` | `string` | Required. Place to search (Tokyo, Japan, etc...) |

## Screenshots
- MySQL example

![image](https://github.com/user-attachments/assets/f87960a1-a76a-492d-aeaf-b557bdd0d3e8)

- Console output examples

![image](https://github.com/user-attachments/assets/06e0eac0-f1a6-4368-a894-8721b0cad054)

- Search Places example

![image](https://github.com/user-attachments/assets/b0ef13b6-ddbc-44d0-a72d-5e510c77daa4)

![image](https://github.com/user-attachments/assets/56e56dcb-823b-4ae0-9861-f05a9d3119a5)


