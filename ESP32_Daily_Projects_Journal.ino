#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "";
const char* password = "";

// Create WebServer object on port 80
WebServer server(80);

// File paths
const char* indexHtmlPath = "/journal.html";

// Buffer for JSON document
StaticJsonDocument<32768> jsonDoc; // Increased size for larger projects

// Forward declarations
void handleRoot();
void handleStatusCheck();
void handleGetProjects();
void handleAddProject();
void handleDeleteProject();
void handleGetEntry();
void handleSaveEntry();
void handleCORS();
void handleNotFound();
void setupServerRoutes();
bool createHtmlFile();

// Function to initialize SPIFFS with better error reporting
bool initSPIFFS() {
  Serial.println("\n=== Initializing SPIFFS ===");
  
  // Try to mount SPIFFS without formatting
  if (!SPIFFS.begin(false)) {
    Serial.println("Failed to mount, trying to format...");
    if (!SPIFFS.format()) {
      Serial.println("ERROR: Format failed!");
      return false;
    }
    if (!SPIFFS.begin(false)) {
      Serial.println("ERROR: Mount after format failed!");
      return false;
    }
  }
  
  // Print SPIFFS info
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  Serial.println("\nSPIFFS Status:");
  Serial.printf("  Total space: %u bytes\n", totalBytes);
  Serial.printf("  Used space: %u bytes\n", usedBytes);
  Serial.printf("  Free space: %u bytes\n", totalBytes - usedBytes);
  Serial.printf("  Space used: %.1f%%\n", (float)usedBytes * 100 / totalBytes);
  
  // Print ESP32 memory info
  Serial.println("\nESP32 Memory Status:");
  Serial.printf("  Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("  Largest free block: %u bytes\n", ESP.getMaxAllocHeap());
  
  // List all files
  Serial.println("\nFiles in SPIFFS:");
  int fileCount = 0;
  
  {
    File root = SPIFFS.open("/");
    if (!root) {
      Serial.println("ERROR: Failed to open root directory");
      return false;
    }
    
    if (!root.isDirectory()) {
      Serial.println("ERROR: Root is not a directory");
      root.close();
      return false;
    }
    
    File file = root.openNextFile();
    while(file) {
      const char* fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("  %s (%d bytes)\n", fileName, fileSize);
      file = root.openNextFile();
      fileCount++;
    }
    root.close();
  }
  
  if (fileCount == 0) {
    Serial.println("  No files found");
  } else {
    Serial.printf("  Found %d files\n", fileCount);
  }
  
  // Create the HTML file if it doesn't exist
  if (!SPIFFS.exists(indexHtmlPath)) {
    if (!createHtmlFile()) {
      Serial.println("ERROR: Failed to create HTML file");
      return false;
    }
    Serial.println("Created initial HTML file");
  }
  
  Serial.println("\n=== SPIFFS Initialization Complete ===");
  Serial.println("SPIFFS mounted successfully");
  return true;
}

// Function to create the HTML file in SPIFFS
bool createHtmlFile() {
  File file = SPIFFS.open(indexHtmlPath, FILE_WRITE);
  if (!file) {
    Serial.println("ERROR: Failed to create HTML file");
    return false;
  }
  
  // The proper way to use raw string literals - note this is one continuous string
  file.print(R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>ESP32 Projects Journal</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --primary-color: #3a86ff;
      --secondary-color: #ff006e;
      --success-color: #38b000;
      --warning-color: #ffbe0b;
      --error-color: #ef233c;
      --dark-color: #1a1a2e;
      --light-color: #f8f9fa;
      --gray-color: #6c757d;
      
      --bg-primary: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      --glass-bg: rgba(255, 255, 255, 0.1);
      --glass-border: rgba(255, 255, 255, 0.2);
      --shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
    }
    
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Poppins', sans-serif;
      transition: all 0.3s ease;
    }
    
    body {
      background: var(--bg-primary);
      color: var(--light-color);
      min-height: 100vh;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      position: relative;
      overflow-x: hidden;
    }
    
    body::before {
      content: '';
      position: absolute;
      top: -100px;
      left: -100px;
      width: 300px;
      height: 300px;
      background: radial-gradient(circle, rgba(255,0,110,0.4) 0%, rgba(255,0,110,0) 70%);
      border-radius: 50%;
      z-index: -1;
    }
    
    body::after {
      content: '';
      position: absolute;
      bottom: -100px;
      right: -100px;
      width: 300px;
      height: 300px;
      background: radial-gradient(circle, rgba(58,134,255,0.4) 0%, rgba(58,134,255,0) 70%);
      border-radius: 50%;
      z-index: -1;
    }
    
    .container {
      width: 100%;
      max-width: 900px;
      backdrop-filter: blur(10px);
      -webkit-backdrop-filter: blur(10px);
      background: var(--glass-bg);
      border-radius: 20px;
      border: 1px solid var(--glass-border);
      box-shadow: var(--shadow);
      padding: 30px;
      overflow: hidden;
      margin-top: 20px;
    }
    
    header {
      text-align: center;
      margin-bottom: 30px;
    }
    
    header h1 {
      font-size: 2.5rem;
      font-weight: 700;
      background: linear-gradient(to right, #3a86ff, #ff006e);
      -webkit-background-clip: text;
      background-clip: text;
      -webkit-text-fill-color: transparent;
      margin-bottom: 10px;
      display: inline-block;
    }
    
    header p {
      color: rgba(255, 255, 255, 0.7);
      font-size: 1.1rem;
    }
    
    .control-panel {
      display: grid;
      grid-template-columns: 1fr;
      gap: 20px;
      margin-bottom: 30px;
    }
    
    .panel-row {
      display: flex;
      gap: 15px;
      align-items: flex-end;
    }
    
    @media (max-width: 768px) {
      .panel-row {
        flex-direction: column;
        align-items: stretch;
      }
    }
    
    .form-group {
      flex: 1;
      margin-bottom: 5px;
    }
    
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: 500;
      color: var(--light-color);
      font-size: 0.95rem;
    }
    
    input, select, textarea, button {
      width: 100%;
      padding: 12px 15px;
      border-radius: 10px;
      border: 1px solid var(--glass-border);
      background: rgba(0, 0, 0, 0.2);
      color: var(--light-color);
      font-size: 1rem;
      outline: none;
    }
    
    input:focus, select:focus, textarea:focus {
      border-color: var(--primary-color);
      box-shadow: 0 0 0 2px rgba(58, 134, 255, 0.3);
    }
    
    .btn {
      cursor: pointer;
      font-weight: 600;
      text-align: center;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 12px 20px;
      border: none;
      transition: all 0.3s;
    }
    
    .btn:hover {
      transform: translateY(-2px);
    }
    
    .btn:active {
      transform: translateY(1px);
    }
    
    .btn-primary {
      background: linear-gradient(45deg, #3a86ff, #4361ee);
      color: white;
    }
    
    .btn-success {
      background: linear-gradient(45deg, #38b000, #4cc9f0);
      color: white;
    }
    
    .btn-danger {
      background: linear-gradient(45deg, #ef233c, #ff006e);
      color: white;
    }
    
    .editor-section {
      margin-bottom: 25px;
    }
    
    textarea {
      width: 100%;
      min-height: 150px;
      resize: vertical;
      font-family: 'Poppins', sans-serif;
      line-height: 1.6;
      margin-top: 10px;
    }
    
    #codeText {
      font-family: 'Courier New', monospace;
      background-color: #161b22;
      color: #58a6ff;
      border: 1px solid #30363d;
      min-height: 200px;
    }
    
    .status-message {
      padding: 15px;
      border-radius: 10px;
      margin: 20px 0 10px 0;
      font-weight: 500;
      display: flex;
      align-items: center;
      justify-content: space-between;
      opacity: 0;
      transform: translateY(20px);
      transition: opacity 0.5s, transform 0.5s;
    }
    
    .status-message.show {
      opacity: 1;
      transform: translateY(0);
    }
    
    .status-success {
      background: rgba(56, 176, 0, 0.2);
      border: 1px solid rgba(56, 176, 0, 0.5);
      color: #a7f3d0;
    }
    
    .status-error {
      background: rgba(239, 35, 60, 0.2);
      border: 1px solid rgba(239, 35, 60, 0.5);
      color: #fda4af;
    }
    
    .status-close {
      background: none;
      border: none;
      color: currentColor;
      font-size: 1.2rem;
      cursor: pointer;
      padding: 0;
      width: auto;
    }
    
    .loading {
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 30px;
      font-size: 1.1rem;
      color: var(--gray-color);
    }
    
    .loading i {
      margin-right: 10px;
      animation: spin 1.5s linear infinite;
    }
    
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    
    .footer {
      margin-top: 40px;
      text-align: center;
      color: rgba(255, 255, 255, 0.5);
      font-size: 0.9rem;
    }
    
    .footer a {
      color: var(--primary-color);
      text-decoration: none;
    }
    
    .actions-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 15px;
      margin-top: 30px;
    }
    
    @media (max-width: 600px) {
      .actions-row {
        flex-direction: column;
      }
      
      .actions-row button {
        width: 100%;
      }
    }
    
    .empty-projects {
      text-align: center;
      padding: 30px;
      border: 1px dashed var(--glass-border);
      border-radius: 10px;
      margin: 20px 0;
    }
    
    .empty-projects i {
      font-size: 3rem;
      color: var(--gray-color);
      margin-bottom: 15px;
    }
    
    .empty-projects p {
      color: var(--gray-color);
    }
    
    /* Animation keyframes */
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .fade-in {
      animation: fadeIn 0.5s ease-out forwards;
    }
    
    .add-project-form {
      display: flex;
      gap: 10px;
    }
    
    @media (max-width: 768px) {
      .add-project-form {
        flex-direction: column;
      }
    }
    
    /* Tooltip */
    .tooltip {
      position: relative;
      display: inline-block;
    }
    
    .tooltip .tooltip-text {
      visibility: hidden;
      width: 200px;
      background-color: rgba(0, 0, 0, 0.9);
      color: #fff;
      text-align: center;
      border-radius: 6px;
      padding: 10px;
      position: absolute;
      z-index: 1;
      bottom: 125%;
      left: 50%;
      transform: translateX(-50%);
      opacity: 0;
      transition: opacity 0.3s;
      font-size: 0.8rem;
    }
    
    .tooltip:hover .tooltip-text {
      visibility: visible;
      opacity: 1;
    }
    
    /* Top status bar */
    .status-bar {
      background: rgba(0, 0, 0, 0.3);
      padding: 10px 20px;
      width: 100%;
      max-width: 900px;
      border-radius: 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
    }
    
    .status-bar .connection-status {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.9rem;
    }
    
    .status-indicator {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      background-color: var(--success-color);
    }
    
    .status-indicator.offline {
      background-color: var(--error-color);
    }
  </style>
</head>
<body>
  <div class="status-bar">
    <div class="connection-status">
      <div id="connectionIndicator" class="status-indicator"></div>
      <span id="connectionStatus">Connected</span>
    </div>
    <div>
      <span id="currentTime"></span>
    </div>
  </div>

  <div class="container fade-in">
    <header>
      <h1><i class="fas fa-microchip"></i> ESP32 Projects Journal</h1>
      <p>Document your project journey and code snippets</p>
    </header>
    
    <div class="control-panel">
      <div class="panel-row">
        <div class="form-group">
          <label for="entryDate"><i class="fas fa-calendar-alt"></i> Date</label>
          <input type="date" id="entryDate" onchange="loadProjects()">
        </div>
        
        <div class="form-group">
          <label for="projectSelect"><i class="fas fa-folder-open"></i> Project</label>
          <select id="projectSelect" onchange="loadEntry()"></select>
        </div>
        
        <div class="form-group tooltip">
          <button onclick="deleteProject()" class="btn btn-danger">
            <i class="fas fa-trash"></i> Delete
          </button>
          <span class="tooltip-text">Delete the selected project and all its entries</span>
        </div>
      </div>
      
      <div class="panel-row">
        <div class="form-group" style="flex-grow: 2;">
          <label for="newProjectName"><i class="fas fa-plus-circle"></i> Add New Project</label>
          <div class="add-project-form">
            <input type="text" id="newProjectName" placeholder="Enter project name...">
            <button onclick="addProject()" class="btn btn-primary">
              <i class="fas fa-plus"></i> Add Project
            </button>
          </div>
        </div>
      </div>
    </div>
    
    <div id="projectContent">
      <div class="editor-section">
        <label for="journalText"><i class="fas fa-book"></i> Journal Entry</label>
        <textarea id="journalText" placeholder="Write your project notes here..."></textarea>
      </div>
      
      <div class="editor-section">
        <label for="codeText"><i class="fas fa-code"></i> Code Snippet</label>
        <textarea id="codeText" placeholder="// Write your code here..."></textarea>
      </div>
      
      <div class="actions-row">
        <button id="saveBtn" onclick="saveEntry()" class="btn btn-success">
          <i class="fas fa-save"></i> Save Entry
        </button>
      </div>
    </div>
    
    <div id="statusMessage" class="status-message"></div>
  </div>
  
  <div class="footer">
    <p>ESP32 Projects Journal &copy; 2025</p>
  </div>

<script>
  // Initialize variables
  let isOnline = true;
  let loadingProjects = false;
  let loadingEntry = false;
  
  // Utility function to format date
  const formatDate = function(date) {
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    return new Date(date).toLocaleDateString(undefined, options);
  };
  
  // Update time in status bar
  const updateTime = function() {
    const now = new Date();
    document.getElementById('currentTime').textContent = now.toLocaleTimeString();
  };
  
  setInterval(updateTime, 1000);
  updateTime();
  
  // Initialize the page
  window.onload = function() {
    const today = new Date().toISOString().split('T')[0];
    document.getElementById('entryDate').value = today;
    checkConnectionStatus();
    loadProjects();
    
    // Set up connection monitoring
    setInterval(checkConnectionStatus, 5000);
  };
  
  // Check connection status
  const checkConnectionStatus = function() {
    fetch('/api/status', { method: 'GET' })
      .then(function(response) {
        if (response.ok) {
          updateConnectionUI(true);
        } else {
          updateConnectionUI(false);
        }
      })
      .catch(function() {
        updateConnectionUI(false);
      });
  };
  
  const updateConnectionUI = function(connected) {
    const indicator = document.getElementById('connectionIndicator');
    const status = document.getElementById('connectionStatus');
    
    isOnline = connected;
    
    if (connected) {
      indicator.classList.remove('offline');
      status.textContent = 'Connected';
    } else {
      indicator.classList.add('offline');
      status.textContent = 'Disconnected';
      showStatusMessage('Connection lost. Changes might not be saved.', true);
    }
  };
  
  // Show status message
  const showStatusMessage = function(message, isError) {
    isError = isError || false;
    const statusElement = document.getElementById('statusMessage');
    statusElement.className = isError ? 
      'status-message status-error show' : 
      'status-message status-success show';
    
    statusElement.innerHTML = 
      '<div>' + (isError ? '<i class="fas fa-exclamation-circle"></i> ' : '<i class="fas fa-check-circle"></i> ') + message + '</div>' +
      '<button class="status-close" onclick="hideStatusMessage()"><i class="fas fa-times"></i></button>';
    
    // Auto hide after 5 seconds
    setTimeout(function() {
      hideStatusMessage();
    }, 5000);
  };
  
  const hideStatusMessage = function() {
    const statusElement = document.getElementById('statusMessage');
    statusElement.classList.remove('show');
  };
  
  // Load projects for the selected date
  const loadProjects = async function() {
    if (loadingProjects) return;
    loadingProjects = true;
    
    const date = document.getElementById('entryDate').value;
    const projectSelect = document.getElementById('projectSelect');
    
    // Show loading state
    projectSelect.innerHTML = '<option value="" disabled selected>Loading...</option>';
    document.getElementById('journalText').value = '';
    document.getElementById('codeText').value = '';
    
    try {
      const response = await fetch('/api/projects?date=' + encodeURIComponent(date), {
        method: 'GET',
        headers: { 'Accept': 'application/json' }
      });
      
      if (!response.ok) {
        throw new Error('HTTP error ' + response.status);
      }
      
      const projects = await response.json();
      updateProjectSelect(projects);
      
      if (projects.length > 0) {
        projectSelect.value = projects[0];
        loadEntry();
      } else {
        showEmptyState();
      }
    } catch (error) {
      console.error('Failed to load projects:', error);
      showStatusMessage('Error loading projects: ' + error.message, true);
      projectSelect.innerHTML = '<option value="" disabled selected>Failed to load projects</option>';
    } finally {
      loadingProjects = false;
    }
  };
  
  // Update project selection dropdown
  const updateProjectSelect = function(projects) {
    const select = document.getElementById('projectSelect');
    select.innerHTML = '';
    
    if (projects.length === 0) {
      select.innerHTML = '<option value="" disabled selected>No projects found</option>';
      return;
    }
    
    for (let i = 0; i < projects.length; i++) {
      const option = document.createElement('option');
      option.value = projects[i];
      option.textContent = projects[i];
      select.appendChild(option);
    }
  };
  
  // Show empty state when no projects are available
  const showEmptyState = function() {
    document.getElementById('journalText').value = '';
    document.getElementById('codeText').value = '';
    document.getElementById('projectSelect').innerHTML = '<option value="" disabled selected>No projects found</option>';
  };
  
  // Add a new project
  const addProject = async function() {
    const date = document.getElementById('entryDate').value;
    const projectInput = document.getElementById('newProjectName');
    const newProject = projectInput.value.trim();
    
    if (!newProject) {
      showStatusMessage('Please enter a project name', true);
      projectInput.focus();
      return;
    }
    
    // Disable input while processing
    projectInput.disabled = true;
    const addBtn = document.querySelector('button[onclick="addProject()"]');
    const originalButtonText = addBtn.innerHTML;
    addBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Adding...';
    addBtn.disabled = true;
    
    try {
      // Step 1: Add the project
      const addResponse = await fetch('/api/projects', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'text/plain'
        },
        body: JSON.stringify({ date: date, project: newProject })
      });
      
      if (!addResponse.ok) {
        throw new Error('Failed to add project: ' + await addResponse.text());
      }
      
      // Step 2: Reload project list
      await loadProjects();
      
      // Step 3: Select the new project
      const projectSelect = document.getElementById('projectSelect');
      projectSelect.value = newProject;
      
      // Step 4: Create initial empty entry
      const saveResponse = await fetch('/api/entry', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'text/plain'
        },
        body: JSON.stringify({
          date: date,
          project: newProject,
          journal: '',
          code: ''
        })
      });
      
      if (!saveResponse.ok) {
        throw new Error('Failed to create entry: ' + await saveResponse.text());
      }
      
      // Success
      projectInput.value = '';
      showStatusMessage('Project "' + newProject + '" added successfully');
      
      // Enable editing
      const journalTextarea = document.getElementById('journalText');
      const codeTextarea = document.getElementById('codeText');
      journalTextarea.value = '';
      codeTextarea.value = '';
      journalTextarea.disabled = false;
      codeTextarea.disabled = false;
      
    } catch (error) {
      console.error('Failed to add project:', error);
      showStatusMessage('Error: ' + error.message, true);
    } finally {
      // Re-enable inputs
      projectInput.disabled = false;
      addBtn.disabled = false;
      addBtn.innerHTML = originalButtonText;
    }
  };
  
  // Load entry for the selected project
  const loadEntry = async function() {
    const date = document.getElementById('entryDate').value;
    const project = document.getElementById('projectSelect').value;
    
    if (!project) {
      showEmptyState();
      return;
    }
    
    if (loadingEntry) return;
    loadingEntry = true;
    
    // Show loading state
    const journalTextarea = document.getElementById('journalText');
    const codeTextarea = document.getElementById('codeText');
    journalTextarea.value = 'Loading...';
    codeTextarea.value = 'Loading...';
    journalTextarea.disabled = true;
    codeTextarea.disabled = true;
    
    try {
      const response = await fetch('/api/entry?date=' + encodeURIComponent(date) + '&project=' + encodeURIComponent(project));
      
      if (!response.ok) {
        throw new Error('HTTP error ' + response.status);
      }
      
      const entry = await response.json();
      journalTextarea.value = entry.journal || '';
      codeTextarea.value = entry.code || '';
    } catch (error) {
      console.error('Failed to load entry:', error);
      journalTextarea.value = '';
      codeTextarea.value = '';
      showStatusMessage('Error loading entry: ' + error.message, true);
    } finally {
      journalTextarea.disabled = false;
      codeTextarea.disabled = false;
      loadingEntry = false;
    }
  };
  
  // Save entry for the selected project
  const saveEntry = async function() {
    const date = document.getElementById('entryDate').value;
    const project = document.getElementById('projectSelect').value;
    
    if (!project) {
      showStatusMessage('No project selected', true);
      return;
    }
    
    const journalTextarea = document.getElementById('journalText');
    const codeTextarea = document.getElementById('codeText');
    const journal = journalTextarea.value;
    const code = codeTextarea.value;
    
    // Don't save if both fields are empty
    if (!journal.trim() && !code.trim()) {
      return;
    }
    
    // Show saving state
    const saveBtn = document.getElementById('saveBtn');
    const originalButtonText = saveBtn.innerHTML;
    saveBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Saving...';
    
    // Disable all inputs while saving
    saveBtn.disabled = true;
    journalTextarea.disabled = true;
    codeTextarea.disabled = true;
    
    try {
      // Normalize line endings and compress whitespace
      const normalizedCode = code
        .replace(/\r\n/g, '\n')
        .replace(/\t/g, '  ')  // Convert tabs to 2 spaces
        .replace(/[ ]{3,}/g, '  ')  // Normalize multiple spaces to 2
        .replace(/\n{3,}/g, '\n\n');  // Normalize multiple newlines to 2
      
      // Prepare the data
      const requestData = {
        date: date,
        project: project,
        journal: journal,
        code: normalizedCode
      };
      
      const response = await fetch('/api/entry', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'text/plain',
          'X-Content-Length': requestData.code.length.toString()
        },
        body: JSON.stringify(requestData)
      });
      
      const responseText = await response.text();
      
      if (!response.ok) {
        throw new Error(responseText || `HTTP error ${response.status}`);
      }
      
      showStatusMessage(`Saved entry for "${project}" (${(requestData.code.length / 1024).toFixed(1)}KB)`);
    } catch (error) {
      console.error('Failed to save entry:', error);
      showStatusMessage('Error saving entry: ' + error.message, true);
    } finally {
      // Restore all input states
      saveBtn.innerHTML = originalButtonText;
      saveBtn.disabled = false;
      journalTextarea.disabled = false;
      codeTextarea.disabled = false;
    }
  };
  
  // Delete the selected project
  const deleteProject = async function() {
    const date = document.getElementById('entryDate').value;
    const project = document.getElementById('projectSelect').value;
    
    if (!project) {
      showStatusMessage('No project selected', true);
      return;
    }
    
    if (!confirm('Are you sure you want to delete project "' + project + '" for ' + formatDate(date) + '?\n\nThis action cannot be undone!')) {
      return;
    }
    
    try {
      const response = await fetch('/api/projects', {
        method: 'DELETE',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ date: date, project: project })
      });
      
      if (!response.ok) {
        throw new Error('HTTP error ' + response.status);
      }
      
      showStatusMessage('Project "' + project + '" deleted successfully');
      await loadProjects();
    } catch (error) {
      console.error('Failed to delete project:', error);
      showStatusMessage('Error deleting project: ' + error.message, true);
    }
  };
  
  // Handle keyboard shortcuts
  document.addEventListener('keydown', function(e) {
    // Only handle shortcuts if we have a project selected
    const project = document.getElementById('projectSelect').value;
    if (!project) return;
    
    // Ctrl+S or Cmd+S to save
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
      e.preventDefault();
      // Only save if we have content
      const journal = document.getElementById('journalText').value;
      const code = document.getElementById('codeText').value;
      if (journal || code) {
        saveEntry();
      }
    }
  });
</script>
</body>
</html>)rawliteral");

  file.close();
  Serial.println("HTML file created successfully");
  return true;
}

// Function to get projects list file path for a specific date
String getProjectsListFilePath(const String& date) {
  String filePath = "/" + date + ".json";
  Serial.print("Generated file path: ");
  Serial.println(filePath);
  return filePath;
}

// Function to get project entry file path
String getProjectEntryFilePath(const String& date, const String& project) {
  // Use a safe filename by replacing any potentially problematic characters
  String safeProjectName = project;
  safeProjectName.replace("/", "_");
  safeProjectName.replace("\\", "_");
  safeProjectName.replace(":", "_");
  safeProjectName.replace(" ", "_"); // Replace spaces with underscores
  return "/" + date + "_" + safeProjectName + ".json";
}

// Function to read projects list for a specific date
bool getProjectsForDate(const String& date, JsonArray& projectsList) {
  String filePath = getProjectsListFilePath(date);
  Serial.println("Reading projects from: " + filePath);
  
  if (!SPIFFS.exists(filePath)) {
    Serial.println("File does not exist, returning empty array");
    jsonDoc.clear();
    projectsList = jsonDoc.to<JsonArray>();
    return true;
  }
  
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  
  size_t size = file.size();
  Serial.printf("File size: %u bytes\n", size);
  
  if (size == 0) {
    Serial.println("File is empty, returning empty array");
    file.close();
    jsonDoc.clear();
    projectsList = jsonDoc.to<JsonArray>();
    return true;
  }
  
  String content = file.readString();
  file.close();
  
  Serial.println("File contents: " + content);
  
  jsonDoc.clear();
  DeserializationError error = deserializeJson(jsonDoc, content);
  if (error) {
    Serial.printf("JSON parse failed: %s\n", error.c_str());
    return false;
  }
  
  projectsList = jsonDoc.as<JsonArray>();
  Serial.println("Successfully read projects list");
  return true;
}

bool saveProjectsForDate(const String& date, JsonArrayConst newProjects) {
  Serial.println("\n=== Starting saveProjectsForDate ====");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  
  // Get existing projects first
  JsonArray existingProjects;
  DynamicJsonDocument doc(4096);
  JsonArray allProjects = doc.to<JsonArray>();
  
  if (getProjectsForDate(date, existingProjects)) {
    // Add existing projects
    for (JsonVariantConst p : existingProjects) {
      if (!p.isNull() && !p.as<String>().isEmpty()) {
        allProjects.add(p.as<String>());
      }
    }
  }
  
  // Add new projects
  for (JsonVariantConst p : newProjects) {
    if (!p.isNull() && !p.as<String>().isEmpty()) {
      bool exists = false;
      // Check if project already exists
      for (JsonVariantConst existing : allProjects) {
        if (existing.as<String>() == p.as<String>()) {
          exists = true;
          break;
        }
      }
      if (!exists) {
        allProjects.add(p.as<String>());
      }
    }
  }
  
  // Get the file path
  String filePath = getProjectsListFilePath(date);
  
  // Remove existing file
  if (SPIFFS.exists(filePath)) {
    SPIFFS.remove(filePath);
  }
  
  // Create new file
  File file = SPIFFS.open(filePath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  
  // Serialize to file
  if (serializeJson(allProjects, file) == 0) {
    Serial.println("Failed to write to file");
    file.close();
    return false;
  }
  
  file.close();
  
  // Verify the write
  file = SPIFFS.open(filePath, "r");
  if (!file) {
    return false;
  }
  
  String content = file.readString();
  file.close();
  
  Serial.println("Written content: " + content);
  return true;
}

// Function to remove a project from the projects list
bool removeProjectFromList(const String& date, const String& projectToRemove) {
  JsonArray projectsList;
  if (!getProjectsForDate(date, projectsList)) {
    return false;
  }
  
  // Create a new document for the updated list
  DynamicJsonDocument updatedDoc(4096);
  JsonArray updatedProjects = updatedDoc.to<JsonArray>();
  
  // Copy all projects except the one to remove
  for (JsonVariantConst p : projectsList) {
    if (!p.isNull() && p.as<String>() != projectToRemove) {
      updatedProjects.add(p.as<String>());
    }
  }
  
  // Save the updated list
  String filePath = getProjectsListFilePath(date);
  
  if (SPIFFS.exists(filePath)) {
    SPIFFS.remove(filePath);
  }
  
  // If we have no projects left, we can just delete the file
  if (updatedProjects.size() == 0) {
    return true;
  }
  
  // Otherwise write the updated list
  File file = SPIFFS.open(filePath, FILE_WRITE);
  if (!file) {
    return false;
  }
  
  if (serializeJson(updatedProjects, file) == 0) {
    file.close();
    return false;
  }
  
  file.close();
  return true;
}

// Handler for adding a new project
void handleAddProject() {
  // Add CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }
  
  String requestBody = server.arg("plain");
  Serial.println("Received project add request: " + requestBody);
  
  DynamicJsonDocument requestDoc(1024);
  DeserializationError error = deserializeJson(requestDoc, requestBody);
  if (error) {
    String errorMsg = "Invalid JSON data: " + String(error.c_str());
    Serial.println(errorMsg);
    server.send(400, "text/plain", errorMsg);
    return;
  }
  
  if (!requestDoc.containsKey("date") || !requestDoc.containsKey("project")) {
    server.send(400, "text/plain", "Missing date or project");
    return;
  }
  
  String date = requestDoc["date"].as<String>();
  String project = requestDoc["project"].as<String>();
  
  if (date.isEmpty() || project.isEmpty()) {
    server.send(400, "text/plain", "Date and project cannot be empty");
    return;
  }
  
  // Create a new array with just the new project
  DynamicJsonDocument newDoc(1024);
  JsonArray newArray = newDoc.to<JsonArray>();
  newArray.add(project);
  
  // Save the project - saveProjectsForDate will handle merging with existing
  if (!saveProjectsForDate(date, newArray)) {
    server.send(500, "text/plain", "Failed to save project list");
    return;
  }

  // Create an empty entry file for the new project
  String entryFilePath = getProjectEntryFilePath(date, project);
  Serial.println("Creating entry file at: " + entryFilePath);
  
  File entryFile = SPIFFS.open(entryFilePath, "w");
  if (!entryFile) {
    Serial.println("Failed to create entry file: " + entryFilePath);
    Serial.printf("SPIFFS Status - Used: %u bytes, Free: %u bytes\n", SPIFFS.usedBytes(), SPIFFS.totalBytes() - SPIFFS.usedBytes());
    server.send(500, "text/plain", "Failed to create entry file");
    return;
  }
  
  // Write empty entry
  entryFile.print("{\"journal\":\"\",\"code\":\"\"}");
  entryFile.close();
  
  Serial.println("Created empty entry file: " + entryFilePath);
  server.send(200, "text/plain", "Project added successfully");
}

// Handler for deleting a project
void handleDeleteProject() {
  // Add CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }
  
  String requestBody = server.arg("plain");
  Serial.println("Received project delete request: " + requestBody);
  
  DynamicJsonDocument requestDoc(1024);
  DeserializationError error = deserializeJson(requestDoc, requestBody);
  if (error) {
    String errorMsg = "Invalid JSON data: " + String(error.c_str());
    Serial.println(errorMsg);
    server.send(400, "text/plain", errorMsg);
    return;
  }
  
  if (!requestDoc.containsKey("date") || !requestDoc.containsKey("project")) {
    server.send(400, "text/plain", "Missing date or project");
    return;
  }
  
  String date = requestDoc["date"].as<String>();
  String project = requestDoc["project"].as<String>();
  
  if (date.isEmpty() || project.isEmpty()) {
    server.send(400, "text/plain", "Date and project cannot be empty");
    return;
  }
  
  // Remove the project from the projects list
  if (!removeProjectFromList(date, project)) {
    server.send(500, "text/plain", "Failed to update project list");
    return;
  }
  
  // Delete the project entry file
  String entryFilePath = getProjectEntryFilePath(date, project);
  if (SPIFFS.exists(entryFilePath)) {
    if (!SPIFFS.remove(entryFilePath)) {
      server.send(500, "text/plain", "Failed to delete project entry file");
      return;
    }
  }
  
  server.send(200, "text/plain", "Project deleted successfully");
}

// Root handler - serves the main HTML page
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
  
  File file = SPIFFS.open(indexHtmlPath, "r");
  if (!file) {
    if (!createHtmlFile()) {
      server.send(500, "text/plain", "Failed to create HTML file");
      return;
    }
    file = SPIFFS.open(indexHtmlPath, "r");
    if (!file) {
      server.send(500, "text/plain", "Failed to open HTML file after creation");
      return;
    }
  }
  
  server.streamFile(file, "text/html");
  file.close();
}

// Status check handler
void handleStatusCheck() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
  server.send(200, "text/plain", "Server is running");
}

// Get projects handler
void handleGetProjects() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
  
  String date = server.arg("date");
  if (date.isEmpty()) {
    server.send(400, "text/plain", "Date parameter is required");
    return;
  }
  
  JsonArray projectsList;
  if (getProjectsForDate(date, projectsList)) {
    String response;
    serializeJson(projectsList, response);
    server.send(200, "application/json", response);
  } else {
    server.send(500, "text/plain", "Failed to get projects");
  }
}

// Get project entry from file
bool getProjectEntryFile(const String& date, const String& project, JsonObject& entry) {
  String filePath = getProjectEntryFilePath(date, project);
  Serial.println("Reading entry from: " + filePath);
  
  if (!SPIFFS.exists(filePath)) {
    Serial.println("Entry file does not exist: " + filePath);
    return false;
  }
  
  File entryFile = SPIFFS.open(filePath, "r");
  if (!entryFile) {
    Serial.println("Failed to open entry file: " + filePath);
    return false;
  }
  
  jsonDoc.clear();
  DeserializationError error = deserializeJson(jsonDoc, entryFile);
  entryFile.close();
  
  if (error) {
    Serial.println("Failed to parse entry JSON: " + String(error.c_str()));
    return false;
  }
  
  entry = jsonDoc.as<JsonObject>();
  return true;
}

// Handler for getting project entry
void handleGetEntry() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
  
  String date = server.arg("date");
  String project = server.arg("project");
  
  if (date.isEmpty() || project.isEmpty()) {
    server.send(400, "text/plain", "Date and project parameters are required");
    return;
  }
  
  JsonObject entry;
  if (getProjectEntryFile(date, project, entry)) {
    String response;
    serializeJson(entry, response);
    server.send(200, "application/json", response);
  } else {
    // If the entry doesn't exist, create an empty one
    jsonDoc.clear();
    JsonObject newEntry = jsonDoc.to<JsonObject>();
    newEntry["journal"] = "";
    newEntry["code"] = "";
    
    String response;
    serializeJson(newEntry, response);
    server.send(200, "application/json", response);
  }
}

// Handler for saving project entry
void handleSaveEntry() {
  // Add CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Content-Length");

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }
  
  String requestBody = server.arg("plain");
  Serial.println("Received save entry request with body length: " + String(requestBody.length()));
  
  jsonDoc.clear();
  DeserializationError error = deserializeJson(jsonDoc, requestBody);
  if (error) {
    String errorMsg = "Invalid JSON data: " + String(error.c_str());
    Serial.println(errorMsg);
    server.send(400, "text/plain", errorMsg);
    return;
  }
  
  // Extract date, project, and entry data
  if (!jsonDoc.containsKey("date") || !jsonDoc.containsKey("project") || 
      !jsonDoc.containsKey("journal") || !jsonDoc.containsKey("code")) {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }
  
  String date = jsonDoc["date"].as<String>();
  String project = jsonDoc["project"].as<String>();
  String journal = jsonDoc["journal"].as<String>();
  String code = jsonDoc["code"].as<String>();
  
  if (date.isEmpty() || project.isEmpty()) {
    server.send(400, "text/plain", "Date and project cannot be empty");
    return;
  }
  
  // Create a new JSON document for the entry
  DynamicJsonDocument entryDoc(32768);
  JsonObject entry = entryDoc.to<JsonObject>();
  entry["journal"] = journal;
  entry["code"] = code;
  
  String filePath = getProjectEntryFilePath(date, project);
  Serial.println("Saving entry to: " + filePath);
  
  // Remove existing file if it exists
  if (SPIFFS.exists(filePath)) {
    if (!SPIFFS.remove(filePath)) {
      Serial.println("Failed to remove existing entry file");
      server.send(500, "text/plain", "Failed to save entry");
      return;
    }
    delay(100); // Give SPIFFS time to delete
  }
  
  File file = SPIFFS.open(filePath, "w");
  if (!file) {
    Serial.println("Failed to create entry file");
    server.send(500, "text/plain", "Failed to save entry");
    return;
  }
  
  // Serialize directly to file to save memory
  if (serializeJson(entry, file) == 0) {
    Serial.println("Failed to write entry file");
    file.close();
    server.send(500, "text/plain", "Failed to save entry");
    return;
  }
  
  file.close();
  Serial.println("Entry saved successfully");
  server.send(200, "text/plain", "Entry saved successfully");
}

// CORS handler
void handleCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Content-Length");
  server.send(200, "text/plain", "");
}

// 404 handler
void handleNotFound() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/plain", "Not found");
}

void setupServerRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatusCheck);
  server.on("/api/projects", HTTP_GET, handleGetProjects);
  server.on("/api/projects", HTTP_POST, handleAddProject);
  server.on("/api/projects", HTTP_DELETE, handleDeleteProject);
  server.on("/api/projects", HTTP_OPTIONS, handleCORS);
  server.on("/api/entry", HTTP_GET, handleGetEntry);
  server.on("/api/entry", HTTP_POST, handleSaveEntry);
  server.on("/api/entry", HTTP_OPTIONS, handleCORS);
  server.onNotFound(handleNotFound);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to start

  Serial.println("\n=== ESP32 Daily Projects Journal Starting ===");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Largest free block: %u bytes\n", ESP.getMaxAllocHeap());

  // Initialize SPIFFS without formatting
  if (!initSPIFFS()) {
    Serial.println("Failed to initialize SPIFFS. Formatting...");
    SPIFFS.format();
    if (!initSPIFFS()) {
      Serial.println("ERROR: Failed to initialize SPIFFS after formatting!");
      return;
    }
  }
  
  // Connect to Wi-Fi with feedback
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed!");
    return;
  }
  
  Serial.println("\nConnected! IP address: " + WiFi.localIP().toString());
  
  // Set up server routes
  setupServerRoutes();
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Journal interface available at: http://" + WiFi.localIP().toString());
}

unsigned long lastWiFiCheck = 0;

void loop() {
  // Handle client requests
  server.handleClient();
  
  // Check WiFi status every 30 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - lastWiFiCheck >= 30000) {
    lastWiFiCheck = currentMillis;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Attempting to reconnect...");
      WiFi.reconnect();
    }
    
    // Log memory status
    Serial.print("Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" bytes, Largest free block: ");
    Serial.print(ESP.getMaxAllocHeap());
    Serial.println(" bytes");
  }
  
  // Small delay to prevent watchdog timeouts
  delay(10);
}