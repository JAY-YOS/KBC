# 🎬 Kaun Banega Crorepati — The Game

A fully functional **KBC (Kaun Banega Crorepati)** trivia game built entirely in **C** as a CGI application, with server-side rendered HTML templates. No frameworks, no databases — just raw C powering a beautiful web-based quiz game.
---

👥 Team Information

- **Jai Singh Rathore** (Team Lead) — University Roll: 2592593
- **Jahnvi Agarwal** — University Roll: 2592592
- **Mihika Negi** — University Roll: 0000000
---

## ✨ Features

- **15-Level Prize Ladder** — Win up to ₹1 Crore, just like the real show
- **3 Lifelines**
  - 🔀 **Flip the Question** — Get a completely new question
  - ½ **50:50** — Eliminate two wrong answers
  - 👥 **Audience Poll** — See what the audience thinks (with realistic poll bars)
- **Safe Havens** — Guaranteed prize checkpoints at Level 5 (₹10,000) and Level 10 (₹3,20,000)
- **Live Trivia Questions** — Fetched in real-time from [The Trivia API](https://the-trivia-api.com/)
- **Adaptive Difficulty** — Easy → Medium → Hard as you climb the ladder
- **Custom SSR Templating Engine** — Written from scratch in C with section-based rendering

---

## 🛠️ Tech Stack

| Layer | Technology |
|-------|-----------|
| **Backend** | C (CGI) |
| **Frontend** | HTML + CSS + Vanilla JS |
| **Server** | Apache (XAMPP) |
| **API** | The Trivia API (v2) |
| **Templating** | Custom SSR engine in C |

---

## 📂 Project Structure

```
├── kbc.c            # Backend — all game logic, templating engine, CGI handler
├── template.html    # Single-file HTML template with all game screens (START, FETCH, BOARD, RESULT)
├── index.html       # Entry point — redirects to the CGI endpoint
├── .gitignore
└── README.md
```

---

## 🚀 Setup & Run

### Prerequisites
- [XAMPP](https://www.apachefriends.org/) (or any Apache server with CGI enabled)
- GCC compiler (MinGW on Windows)

### 1. Compile
```bash
gcc kbc.c -o kbc.cgi
```

### 2. Deploy
Copy the compiled binary and template to your XAMPP installation:
```bash
# Copy the CGI binary
cp kbc.cgi C:/xampp/cgi-bin/

# Copy the template (the path is hardcoded in kbc.c)
cp template.html C:/xampp/htdocs/

# Copy the redirect page
cp index.html C:/xampp/htdocs/
```

### 3. Play
Start Apache from XAMPP and visit:
```
http://localhost/cgi-bin/kbc.cgi
```

---

## 🎮 How It Works

1. **Player enters their name** on the start screen
2. **A question is fetched** from The Trivia API via client-side JavaScript
3. **The question is submitted** back to the C backend, which renders the game board using the template engine
4. **Player selects an answer** (or uses a lifeline) — the form POSTs back to the CGI
5. **The backend evaluates** the answer, updates the game state, and renders the result screen
6. **Repeat** until the player wins ₹1 Crore, answers wrong, or quits

> The entire game state is carried through hidden HTML form fields — no cookies, no sessions, no database. Pure stateless CGI.

---

## 📜 License

This project is open source and available for educational purposes.
