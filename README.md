# COMP3016 project - Emerge

**Emerge** is a top-down 2D survival prototype inspired by _Evolve_ and old MAME-era arcade games.
You play as a monster that feeds, evolves through multiple stages, and ultimately escapes a cave system while hunted by AI-controlled squads.

---

Video
**YouTube (Unlisted):** [[https://youtu.be/qjOuXL5nJbE](https://youtu.be/qjOuXL5nJbE)/]()
*(This same video is also provided in the submission zip for the DLE requirement.)*

---

Gameplay Description
The player must go through various stages (Grow, Hunt, Escape) each with various objectives:
| Phase | Objective | Description |
|-------|------------|-------------|
| Grow  | Feed on wildlife to evolve. | Each evolution unlocks a new ability (dash, boulder, slam). |
| Hunt  | Eliminate the AI hunter squad. | Hunters use vision cones, ranged rifles, and squad intel sharing. |
| Escape | Break the border wall to reach the surface. | Smash through the cave walls using high-level abilities. |

---

Controls
| Key | Action |
|-----|---------|
| **WASD** | Move |
| **Mouse** | Aim / Bite Direction |
| **Left Click** | Bite |
| **Shift** | Dash (Stage 2+) |
| **Right Click** | Boulder Attack (Stage 3+) |
| **Q** | Slam (Stage 4+) |
| **E** | Evolve |
| **ESC** | Pause / Menu |

---

Dependencies Used
- **[Raylib 4.x](https://www.raylib.com/)** — graphics, input, and window handling  
- No external engines or asset libraries beyond Raylib 

---

Use of AI
- Generative AI was used when determining best technologies to utilise
- It was sometimes used for bug fixing where the time to solve errors surpassed the 60 minute mark


---

Game Programming Patterns
- **Entity Component–like structure:** Separate `Player`, `Hunter`, `Animal`, `Tilemap`, and `Boulder` classes  
- **Finite State Machine (FSM):**  
  - Player stages → abilities unlocked by state  
  - Hunter AI → idle, chase, shoot, investigate  
- **Camera Shake Pattern:** temporary camera offset decaying over time  
- **Object Pooling:** reusing `Bullet` and `Boulder` vectors  
- **Observer “Squad Intel” system:** hunters share knowledge of player location

---

Mechanics Implementation
| Mechanic | Implementation Detail |
|-----------|------------------------|
| **Evolution** | Player gains “food” count; when threshold met, presses `E` to evolve; temporary vulnerability during transform. |
| **Vision/Lighting** | Full-screen light mask with player & hunter light cones rendered to a secondary texture. |
| **Collision** | Tile-based collision via `resolveCollision()` sampling solid tiles. |
| **Projectiles** | `Bullet` struct with team flags and damage checks per frame. |
| **Abilities** | Dash (velocity burst + invuln), Boulder (projectile with AoE), Slam (AoE + map carving). |
| **Camera Shake** | Timed amplitude-based offset added to camera target. |

---

Exception Handling and Testing
- Checked all file and texture loads for existence (`FileExists()` and `IsFileReady()`).  
- Prevented division-by-zero in vector normalization.  
- Added i-frame cooldown to prevent multi-hit deaths.  
- Verified tile collisions and projectile AoE with debug overlays

---

Evaluation
> _"Emerge"_ demonstrates procedural cave generation, multi-stage player evolution, and AI coordination in a 2D environment built entirely with C++ and Raylib.  
It met its prototype goals and runs standalone without any IDE.  

If I were to continue development:
- Add sound design, particle effects, and animations  
- Expand hunter and monster weapon/ability variety  
- Eventually implement local co-op or multiplayer  

