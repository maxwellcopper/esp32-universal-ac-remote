const express = require("express"); //include library express

const app = express();
const PORT = 3000;

app.use(express.json());

app.get("/", (req, res) => {
  res.send("ESP32 REST API server is running");
});

app.post("/current", (req, res) => {
  console.log("Received current data:");
  console.log(req.body); //print json body from client

  res.json({ //give response to client in json
    status: "ok",
    message: "Current data received",
    data: req.body
  });
});

app.post("/status", (req, res) => {
  console.log("Received AC status:");
  console.log(req.body);

  res.json({
    status: "ok",
    message: "AC status received",
    data: req.body
  });
});

app.listen(PORT, "0.0.0.0", () => { //0.0.0.0 means listen from all not from spesific ip 
  console.log(`Server running on port ${PORT}`); 
});