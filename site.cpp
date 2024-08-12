#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "SSID PASSWORD";

// Create a WebServer object
WebServer server(80);

struct Post {
  String title;
  String content;
  unsigned long timestamp;
  int likes;
  String comments;
};

// Array to store posts (in RAM)
Post posts[10];
int postCount = 0;

// HTML content
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Message Board</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div id="app">
        <h1>Message Board</h1>

        <input type="text" id="searchBar" placeholder="Search posts...">

        <form id="postForm">
            <input type="text" id="postTitle" placeholder="Title" required>
            <textarea id="postContent" placeholder="Write your post here..." required></textarea>
            <button type="submit">Post</button>
        </form>

        <div id="postsContainer">
            <!-- Posts will be dynamically added here by JavaScript -->
        </div>
    </div>

    <script src="script.js"></script>
</body>
</html>
)rawliteral";

// CSS content
const char styles_css[] PROGMEM = R"rawliteral(
body {
    font-family: Arial, sans-serif;
    background-color: #f4f4f4;
    margin: 0;
    padding: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    flex-direction: column;
}

#app {
    width: 80%;
    max-width: 600px;
    background: #fff;
    padding: 20px;
    border-radius: 8px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
}

h1 {
    text-align: center;
    color: #333;
}

form {
    display: flex;
    flex-direction: column;
    margin-bottom: 20px;
}

input, textarea {
    margin-bottom: 10px;
    padding: 10px;
    border: 1px solid #ccc;
    border-radius: 4px;
}

button {
    padding: 10px;
    background-color: #28a745;
    color: #fff;
    border: none;
    border-radius: 4px;
    cursor: pointer;
}

button:hover {
    background-color: #218838;
}

#postsContainer {
    margin-top: 20px;
}

.post {
    background: #f9f9f9;
    padding: 15px;
    border: 1px solid #ddd;
    border-radius: 4px;
    margin-bottom: 10px;
}

.post h2 {
    margin: 0 0 10px;
    color: #333;
}

.post p {
    margin: 0;
    color: #555;
}

.post .timestamp {
    font-size: 0.8em;
    color: #999;
    margin-top: 5px;
}

.comment {
    background: #f1f1f1;
    padding: 10px;
    border: 1px solid #ccc;
    border-radius: 4px;
    margin-top: 10px;
    margin-left: 20px;
}

.comment p {
    margin: 0;
    color: #555;
}

.comment .timestamp {
    font-size: 0.8em;
    color: #999;
    margin-top: 5px;
}

#searchBar {
    margin-bottom: 20px;
    padding: 10px;
    width: 100%;
    border: 1px solid #ccc;
    border-radius: 4px;
}
)rawliteral";

// JavaScript content
const char script_js[] PROGMEM = R"rawliteral(
document.addEventListener('DOMContentLoaded', () => {
    const postForm = document.getElementById('postForm');
    const postsContainer = document.getElementById('postsContainer');
    const searchBar = document.getElementById('searchBar');

    const fetchPosts = async () => {
        const response = await fetch('/getPosts');
        const posts = await response.json();
        displayPosts(posts);
    };

    const displayPosts = (posts) => {
        postsContainer.innerHTML = '';
        posts.forEach((post, index) => {
            const postElement = document.createElement('div');
            postElement.className = 'post';
            postElement.innerHTML = `
                <h2>${post.title}</h2>
                <p>${post.content}</p>
                <p class="timestamp">${new Date(post.timestamp).toLocaleString()}</p>
                <button onclick="toggleCommentForm(${index})">Add Comment</button>
                <button onclick="addLike(${index})">Like (${post.likes})</button>
                <div id="commentsContainer-${index}">
                    ${post.comments ? post.comments.map(comment => `
                        <div class="comment">
                            <p>${comment.content}</p>
                            <p class="timestamp">${new Date(comment.timestamp).toLocaleString()}</p>
                        </div>
                    `).join('') : ''}
                </div>
                <div id="commentFormContainer-${index}" class="comment-form-container" style="display: none;">
                    <textarea id="commentContent-${index}" placeholder="Write your comment here..." required></textarea>
                    <button onclick="addComment(${index})">Post Comment</button>
                </div>
            `;
            postsContainer.appendChild(postElement);
        });
    };

    postForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const title = document.getElementById('postTitle').value;
        const content = document.getElementById('postContent').value;

        await fetch('/post', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `title=${encodeURIComponent(title)}&content=${encodeURIComponent(content)}`
        });

        document.getElementById('postTitle').value = '';
        document.getElementById('postContent').value = '';
        fetchPosts();
    });

    fetchPosts();
});
)rawliteral";

// Handle root URL
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

// Handle CSS
void handleCss() {
  server.send_P(200, "text/css", styles_css);
}

// Handle JavaScript
void handleJs() {
  server.send_P(200, "application/javascript", script_js);
}

// Handle fetching posts
void handleGetPosts() {
  String jsonResponse = "[";
  for (int i = 0; i < postCount; i++) {
    jsonResponse += "{\"title\":\"" + posts[i].title + "\",\"content\":\"" + posts[i].content + "\",\"timestamp\":" + String(posts[i].timestamp) + ",\"likes\":" + String(posts[i].likes) + "}";
    if (i < postCount - 1) jsonResponse += ",";
  }
  jsonResponse += "]";
  server.send(200, "application/json", jsonResponse);
}

// Handle posting new posts
void handlePost() {
  if (server.hasArg("title") && server.hasArg("content")) {
    if (postCount < 10) {
      posts[postCount].title = server.arg("title");
      posts[postCount].content = server.arg("content");
      posts[postCount].timestamp = millis();
      posts[postCount].likes = 0;
      posts[postCount].comments = "";
      postCount++;
    }
    server.send(200, "text/plain", "Post received");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println(WiFi.localIP());

  // Define server routes
  server.on("/", handleRoot);
  server.on("/styles.css", handleCss);
  server.on("/script.js", handleJs);
  server.on("/getPosts", HTTP_GET, handleGetPosts);
  server.on("/post", HTTP_POST, handlePost);

  // Start server
  server.begin();
}

void loop() {
  server.handleClient();
}
