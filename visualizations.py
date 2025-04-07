import pandas as pd
import matplotlib.pyplot as plt
import os

# Create 'plots' directory if it doesn't exist
plots_dir = "plots_incremental"
if not os.path.exists(plots_dir):
    os.makedirs(plots_dir)

# Load data
data = pd.read_csv("results_incremental.csv")

# Define difficulties and sizes
difficulties = ['Easy', 'Medium', 'Hard']
sizes = [20, 50, 100, 200, 300]  # Tested sizes

# 1. Line Plots with Markers: Adaptation Time vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Difficulty'] == difficulty) &
                      (data['Approach'] == approach)].sort_values('Size')
        if not subset.empty:
            plt.plot(subset['Size'], subset['AdaptTimePerStep'], marker='o', label=approach,
                     linewidth=2, markersize=8)

    plt.xlabel('Maze Size')
    plt.ylabel('Adaptation Time per Step (s)')
    plt.title(f'Adaptation Time per Step vs. Maze Size ({difficulty} Difficulty, 50 Steps)')
    plt.xticks(sizes)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/adapt_time_line_{difficulty.lower()}.png")
    plt.close()

# 2. Line Plots: Success Rate vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Difficulty'] == difficulty) &
                      (data['Approach'] == approach)].sort_values('Size')
        if not subset.empty:
            plt.plot(subset['Size'], subset['AvgSuccessRate'] * 100, marker='o', label=approach,
                     linewidth=2, markersize=8)

    plt.xlabel('Maze Size')
    plt.ylabel('Average Success Rate (%)')
    plt.title(f'Avg Success Rate vs. Maze Size ({difficulty} Difficulty, 50 Steps)')
    plt.xticks(sizes)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/success_rate_line_{difficulty.lower()}.png")
    plt.close()

# 3. Line Plots: Avg Path Length vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Difficulty'] == difficulty) &
                      (data['Approach'] == approach)].sort_values('Size')
        if not subset.empty:
            plt.plot(subset['Size'], subset['AvgPathLength'], marker='o', label=approach,
                     linewidth=2, markersize=8)

    plt.xlabel('Maze Size')
    plt.ylabel('Average Path Length')
    plt.title(f'Avg Path Length vs. Maze Size ({difficulty} Difficulty, 50 Steps)')
    plt.xticks(sizes)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/avg_path_length_line_{difficulty.lower()}.png")
    plt.close()

# 4. Scatter Plots: Success Rate vs. Adaptation Time for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(12, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Difficulty'] == difficulty) &
                      (data['Approach'] == approach)]
        if not subset.empty:
            plt.scatter(subset['AdaptTimePerStep'], subset['AvgSuccessRate'] * 100,
                        s=subset['Size'] * 5, label=approach, alpha=0.5)

    plt.xlabel('Adaptation Time per Step (s)')
    plt.ylabel('Average Success Rate (%)')
    plt.title(f'Avg Success Rate vs. Adaptation Time per Step ({difficulty} Difficulty, 50 Steps)')
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/success_vs_time_scatter_{difficulty.lower()}.png")
    plt.close()