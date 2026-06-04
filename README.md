# 🚉 Train Platform Management System

A C-based Train Platform Management System that efficiently determines the minimum number of railway platforms required to accommodate incoming and outgoing trains without delays.

The project applies the **Greedy Algorithm** technique to optimize platform allocation, minimize scheduling conflicts, and improve resource utilization in transportation systems.

---

## 📖 Project Overview

Railway stations frequently face scheduling conflicts when multiple trains arrive and depart within overlapping time intervals. Efficient platform allocation is essential to avoid train delays and ensure smooth station operations.

This project analyzes train arrival and departure schedules and computes the minimum number of platforms required so that no train has to wait.

---

## 🎯 Objectives

* Determine the minimum number of platforms required.
* Eliminate train waiting time caused by platform shortages.
* Detect scheduling conflicts between trains.
* Demonstrate the practical application of Greedy Algorithms.
* Optimize resource allocation in transportation systems.

---

## ⚙️ Algorithm Used

### Greedy Algorithm for Platform Allocation

The system follows these steps:

1. Read train arrival and departure times.
2. Sort arrival times and departure times separately.
3. Compare upcoming arrivals with current departures.
4. Allocate a new platform if overlap occurs.
5. Free a platform when a train departs.
6. Track the maximum number of platforms used simultaneously.
7. Display the minimum platforms required.

This approach ensures efficient scheduling with optimal resource utilization.

---

## 🔄 Workflow

```text
Start
  │
  ▼
Input Train Details
  │
  ▼
Sort Arrival & Departure Times
  │
  ▼
Compare Arrival and Departure
  │
  ├── Overlap → Allocate New Platform
  │
  └── No Overlap → Free Platform
  │
  ▼
Update Platform Count
  │
  ▼
Generate Conflict Report
  │
  ▼
Display Results
  │
  ▼
End
```

---

## ✨ Features

* Minimum platform calculation
* Train schedule analysis
* Conflict detection
* Efficient resource allocation
* Greedy Algorithm implementation
* Simple and lightweight design
* Suitable for scheduling and optimization studies

---

## 📊 Sample Input

```text
Train    Arrival    Departure
T1       09:00      09:30
T2       09:10      10:00
T3       09:40      10:30
```

### Sample Output

```text
Minimum Platforms Required = 2

Train Platform
T1    P1
T2    P2
T3    P1

Conflict Detected:
T1 and T2 overlap between 09:10 and 09:30
```

---

## 🧠 Applications

This scheduling technique can be applied in:

* Railway Management Systems
* Airport Gate Scheduling
* Bus Terminal Scheduling
* Event Scheduling Systems
* Resource Allocation Problems
* Traffic Management Systems

---

## 📈 Complexity Analysis

| Operation               | Complexity |
| ----------------------- | ---------- |
| Sorting Arrival Times   | O(n log n) |
| Sorting Departure Times | O(n log n) |
| Platform Calculation    | O(n)       |
| Total Complexity        | O(n log n) |

### Space Complexity

```text
O(1)
```

(excluding input storage)

---

## 🛠️ Technologies Used

* Programming Language: C
* Algorithm: Greedy Algorithm
* Data Structures: Arrays
* Development Environment: GCC / VS Code

---

## 🚀 How to Run

Compile:

```bash
gcc main.c -o platform_manager
```

Run:

```bash
./platform_manager
```

---

## 👥 Team Members

| Name              | Roll Number |
| ----------------- | ----------- |
| Pranjal Das Sarma | 27631724005 |
| Kunal Das         | 27631725018 |
| Simi Mallick      | 27631725022 |
| Debmalya Guria    | 27631724004 |
| Chandan Pal       | 27631725023 |

---

## 🎓 Academic Information

**Course:** Design and Analysis of Algorithms

**Department:** CSE-CS

**Semester:** 4th Semester

**Academic Year:** 2025–2026

**Faculty Guide:** Ms. Kaifa Sultana

---

## 🔮 Future Enhancements

* Real-time train schedule integration
* Dynamic platform assignment
* Graphical user interface
* Railway network visualization
* AI-assisted scheduling recommendations
* Large-scale station simulation

---

## 📂 Project Structure

```text
smart-traffic-management-system/
│
├── bin/                          # Generated executables after compilation
│
├── build/                        # Intermediate build artifacts and object files
│
├── data/                         # Sample datasets used for testing
│   └── sample_traffic.csv        # Example train arrival and departure schedule
│
├── docs/                         # Project documentation
│   ├── Project_Report.pdf        # Complete academic project report
│   └── flowchart.png             # Workflow diagram of the scheduling process
│
├── include/                      # Header files
│   └── scheduling.h              # Function declarations and constants
│
├── scripts/                      # Utility and automation scripts
│   └── setup_env.sh              # Environment setup helper script
│
├── src/                          # Core source code
│   └── main.c                    # Greedy platform allocation implementation
│
├── tests/                        # Test cases and validation modules
│   └── test.scheduling.c         # Scheduling logic test suite
│
├── .gitignore                    # Git ignore rules
│
├── CONTRIBUTORS.md               # Team member information and contributions
│
├── LICENSE                       # Project license information
│
└── README.md                     # Project overview and documentation

!! Note that right now only the /src & /docs are in a condition for use , other files are junk data !! 

```
## 🙏 Acknowledgement

We sincerely thank our faculty guide, Ms. Kaifa Sultana, for her valuable guidance and support throughout this project. We are also grateful to our department and institution for providing the necessary resources and environment for successful project development.

---

## 📚 References

1. Introduction to Algorithms – Cormen, Leiserson, Rivest & Stein
2. Data Structures and Algorithms
3. GeeksforGeeks – Platform Allocation Problem
4. Railway Scheduling Research Papers
5. GCC Documentation

```

**⭐ If you find this project useful, consider giving it a star.**
```
