import pandas as pd
import matplotlib.pyplot as plt
import os

# Create 'plots' directory if it doesn't exist
plots_dir = "plots_20x20_new"
if not os.path.exists(plots_dir):
    os.makedirs(plots_dir)

# Load data
data = pd.read_csv("results_20x20_new.csv")

# Define difficulties and sizes
difficulties = ['Easy', 'Medium', 'Hard']
sizes = [10, 20, 50, 100, 200, 300]  # Tested sizes

# 1. Line Plots with Markers: Adaptation Time vs. Maze Size for Each Difficulty and Number of Changes
# Get unique change levels (excluding 0)
change_levels = sorted(data[data['Changes'] > 0]['Changes'].unique())

for difficulty in difficulties:
    for changes in change_levels:
        plt.figure(figsize=(10, 6))
        for approach in data['Approach'].unique():
            subset = data[(data['Difficulty'] == difficulty) &
                          (data['Changes'] == changes)].groupby(['Approach', 'Size'])['AdaptTime'].mean().reset_index()
            subset = subset[subset['Approach'] == approach]
            subset = subset[subset['Size'].isin(sizes)]
            if not subset.empty:  # Only plot if there's data for this change level
                plt.plot(subset['Size'], subset['AdaptTime'], marker='o', label=approach, linewidth=2, markersize=8)

        if plt.gca().has_data():  # Save only if there's plotted data
            plt.xlabel('Maze Size')
            plt.ylabel('Adaptation Time (s)')
            plt.title(f'Adaptation Time vs. Maze Size ({difficulty} Difficulty, {changes} Changes)')
            plt.xticks(sizes)
            plt.grid(axis='y', linestyle='--', alpha=0.7)
            plt.legend()
            plt.tight_layout()
            plt.savefig(f"{plots_dir}/adapt_time_line_{difficulty.lower()}_{changes}_changes.png")
        plt.close()

# 2. Line Plots: Success Rate vs. Number of Changes for Each Size and Difficulty
for size in sizes:
    for difficulty in difficulties:
        plt.figure(figsize=(10, 6))
        for approach in data['Approach'].unique():
            subset = data[(data['Size'] == size) & (data['Difficulty'] == difficulty)]
            if not subset.empty:  # Only plot if there's data for this size
                plt.plot(subset[subset['Approach'] == approach]['Changes'],
                         subset[subset['Approach'] == approach]['SuccessRate'] * 100,
                         label=approach, marker='o', linewidth=2, markersize=8)
        if plt.gca().has_data():  # Save only if there's plotted data
            plt.xlabel('Number of Changes')
            plt.ylabel('Success Rate (%)')
            plt.title(f'Success Rate vs. Changes ({size}x{size}, {difficulty} Difficulty)')
            plt.grid(axis='y', linestyle='--', alpha=0.7)
            plt.legend()
            plt.tight_layout()
            plt.savefig(f"{plots_dir}/success_rate_line_{size}x{size}_{difficulty.lower()}.png")
        plt.close()

# 3. Line Plots: Avg Path Length vs. Changes, Faceted by Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(15, 5))
    for i, size in enumerate(sizes):
        plt.subplot(1, len(sizes), i + 1)
        subset = data[(data['Difficulty'] == difficulty) & (data['Size'] == size) & (data['Changes'] > 0)]
        for approach in data['Approach'].unique():
            sub_subset = subset[subset['Approach'] == approach]
            plt.plot(sub_subset['Changes'], sub_subset['AvgPathLength'], marker='o', label=approach)
        plt.title(f'Size {size}x{size}')
        plt.xlabel('Number of Changes')
        plt.ylabel('Avg Path Length')
        if i == len(sizes) - 1:  # Legend on the last subplot
            plt.legend()
    plt.suptitle(f'Avg Path Length vs. Changes ({difficulty} Difficulty)', y=1.05)
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/avg_path_length_line_{difficulty.lower()}.png")
    plt.close()

# 4. Scatter Plots: Success Rate vs. Adaptation Time for Each Number of Changes
change_levels = sorted(data[data['Changes'] > 0]['Changes'].unique())  # e.g., [1, 5, 10, 20, 50]

for changes in change_levels:
    plt.figure(figsize=(12, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Changes'] == changes) & (data['Approach'] == approach)]
        plt.scatter(subset['AdaptTime'],
                    subset['SuccessRate'] * 100,
                    s=subset['Size'] * 5,  # Smaller multiplier (was * 10)
                    label=approach,
                    alpha=0.5)
    plt.xlabel('Adaptation Time (s)')
    plt.ylabel('Success Rate (%)')
    plt.title(f'Success Rate vs. Adaptation Time ({changes} Changes)')
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/success_vs_time_scatter_{changes}_changes.png")
    plt.close()
