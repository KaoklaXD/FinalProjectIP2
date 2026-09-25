# KVIS Open House Smart Management System (FinalProjectIP2)

A comprehensive graph-based campus navigation and smart exhibition scheduling system developed in C for Kamnoetvidya Science Academy (KVIS) Open House events.

---

## 🚀 Quick Start Guide

### 1. Compilation
Build all three executables (`staff`, `visitor`, `test`) using `make`:
```bash
make clean
make all
```

To build individual binaries:
```bash
make staff    # Builds staff administration executable
make visitor  # Builds visitor guidance executable
```

---

## 📖 User Tutorial

### Part 1: Staff Module (`./staff`)
The Staff module allows event coordinators to configure exhibition checkpoints, assign room numbers, and manage seat capacities across synchronized parallel rounds.

1. **Launch the Staff application**:
   ```bash
   ./staff
   ```
2. **Main Menu**:
   - `[1] Display Existing Checkpoints`: Shows all configured rooms, IDs, and seat capacities across rounds.
   - `[2] Add / Configure a Checkpoint`:
     - Enter Checkpoint ID (e.g. `1`).
     - Enter Checkpoint Name (e.g. `AI&Robotics`).
     - Enter Room Number (e.g. `122`).
       *Note: Only numbered rooms (e.g. 112, 122, 212, 312) and auditoriums (`audi1`, `audi2`) are eligible as checkpoints. Utility nodes such as stairs, lifts, restrooms, and corridors are automatically blocked.*
     - Set number of parallel rounds and seat capacity per round.
   - `[3] Save & Exit`: Writes updates to `checkpoints.txt`.

---

### Part 2: Visitor Module (`./visitor`)
The Visitor module provides personalized planning for individuals and groups attending the Open House.

Launch the Visitor application:
```bash
./visitor
```

You will be greeted by the **Main Visitor Menu**:
```text
========================================================
                     VISITOR MENU                       
========================================================
 [1] Smart Capacity-Aware Itinerary (Continuous Selection & Rounds)
 [2] Free Exploration Tour (Pick Checkpoints Freely & Find Optimal Route)
 [3] Exit
```

---

#### 🌟 Mode 1: Smart Capacity-Aware Itinerary
Use this mode when you want to schedule activities into time slots while ensuring guaranteed seating and avoiding crowded rooms.

1. **Group Registration**:
   - Enter group representative name (e.g. `Wood` $\to$ personalized as **`Wood's Group`**).
   - Enter group size (number of visitors).

2. **Daily Schedule Overview**:
   - Shows parallel synchronized activity rounds and the empty **Lunch Break (Round 4: 12:00 – 13:00)**.

3. **Choose Planning Method**:
   - **`[1] Smart Preference Optimizer` (Recommended)**:
     - View exhibition checkpoints and their seat availability.
     - Choose how many checkpoints you wish to visit (e.g. `3`).
     - Enter your desired Checkpoint IDs **in your preferred order of visit**.
     - **Automatic Feasibility Verification**:
       - **Exact Order Feasible?** If each checkpoint has space in your requested order, the system **preserves your exact sequence** without changes!
       - **Capacity Conflict?** If a room is full in that slot, the **Hungarian Algorithm ($O(k^3)$)** automatically reorders the rounds to minimize your schedule displacement while guaranteeing open seats.
       - **No Possible Round?** If any checkpoint is 100% full across the entire day, the system alerts you and prompts to either **[1] Choose a replacement checkpoint** or **[2] Keep that slot as Free Time**.
     - Immediate seat deduction and persistence to `checkpoints.txt`.
     - Confirmed itinerary summary plan & optional step-by-step walking guidance!
   - **`[2] Manual Step-by-Step Round Selection`**:
     - Manually pick checkpoints round-by-round.
     - If a room is full, Hungarian fallback suggests the closest available alternative. You can accept or choose another checkpoint.

---

#### 🗺️ Mode 2: Free Exploration Tour (Capacity-Independent Shortest Route)
Use this mode when you want to visit a specific set of exhibitions regardless of room capacities, and want the **mathematically shortest walking route**.

1. **Enter Group Details** & select starting location (default: `0` for Main Entrance).
2. **Select Checkpoints**: Enter any number of exhibition IDs without seat restrictions.
3. **Shortest Path Optimization**:
   - Runs `find_optimal_tour` (all-pairs Dijkstra + branch-and-bound permutation search in RAM).
   - Displays the **Optimal Visiting Sequence** minimizing total walking distance.
   - Displays **Efficiency Benefit** (meters saved compared to arbitrary order).
   - Provides **Turn-by-Turn Corridor & Stair Guidance** navigating across floors (passing Level 2 stairs when moving between Level 1 and Level 3).
   - Prints a Confirmed Tour Summary Table.

---


---

## 📁 Key File Structure
- `Visitor_input.c`: Visitor interface, preference optimizer, and tour guidance.
- `Staff_input.c`: Staff checkpoint setup and round management.
- `recommendation.c` / `recommendation.h`: Graph optimization engines (Hungarian algorithm, Dijkstra tour optimizer, Kuhn-Munkres matching).
- `storage.c` / `storage.h`: File parser for `checkpoints.txt` and `distance.txt`.
- `graph.c` / `graph.h`: Graph data structures, adjacency lists, and pathfinding algorithms.
- `checkpoints.txt`: Live capacity database across parallel rounds.
- `distance.txt`: Campus map corridor and stair connections (all 10.0m standardized edges).
