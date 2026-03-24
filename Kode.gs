function testDoGet() {
  // 1. Create a "Fake" event object
  var e = {
    parameter: {
      action: "completeTask",
      id: "QmtVcm1HcEswaTJBVTVDYw", // Get an ID from your previous list fetch
    },
  };

  // 2. Call your doGet and capture the result
  var response = doGet(e);

  // 3. Log the output to the console
  console.log("Response Content: " + response.getContent());
}

function doGet(e) {
  var action = e.parameter.action;
  var id = e.parameter.id;

  if (action == "fetchTask") {
    return fetchTask();
  }

  if (action == "completeTask") {
    return completeTask(id);
  }
}

function fetchTask() {
  console.log("FETCHING");
  const listName = "ESP";

  try {
    // 1. Find the Task List Safely
    var options = {
      showCompleted: false, // This tells Google NOT to send done tasks
      showHidden: false, // This hides archived/old tasks
    };
    const allLists = Tasks.Tasklists.list().items || [];
    const targetList = allLists.find((item) => item.title === listName);

    if (!targetList) {
      return createJsonResponse([
        { title: "Error: List '" + listName + "' not found" },
      ]);
    }

    // 2. Fetch Tasks from that ID
    const taskData = Tasks.Tasks.list(targetList.id, options).items || [];

    // 3. Clean and Minify for ESP32
    const res = taskData.map((item) => ({
      i: item.id,
      t: item.title || "No Title",
      d: item.due,
    }));

    console.log(taskData);
    console.log(res);

    return createJsonResponse(res);
  } catch (e) {
    return createJsonResponse([{ t: "Error: " + e.message }]);
  }
}

function completeTask(id) {
  console.log("COMPLETE TASK");
  try {
    const listName = "ESP";
    const allLists = Tasks.Tasklists.list().items || [];
    const targetList = allLists.find((item) => item.title === listName);

    if (!targetList) {
      return createJsonResponse([
        { title: "Error: List '" + listName + "' not found" },
      ]);
    }
    Tasks.Tasks.patch({ status: "completed" }, targetList.id, id);
    return createJsonResponse({ s: true });
  } catch (e) {
    return createJsonResponse({ s: false, e: "Error: " + e.message });
  }
}

// Helper to keep code clean
function createJsonResponse(data) {
  return ContentService.createTextOutput(JSON.stringify(data)).setMimeType(
    ContentService.MimeType.JSON,
  );
}
