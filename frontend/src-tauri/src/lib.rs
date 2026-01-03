// Sudoku Solver Tauri Application
// Uses WebAssembly for high-performance puzzle solving

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
