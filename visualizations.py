import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# Load data
data = pd.read_csv("results.csv")

# Define difficulties
difficulties = ['Easy', 'Medium', 'Hard']
sizes = [10, 20, 50, 100, 200, 300]  # Tested sizes

# 1. Line Plots with Markers: Adaptation Time vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Difficulty'] == difficulty) & (data['Changes'] > 0)].groupby(['Approach', 'Size'])['AdaptTime'].mean().reset_index()
        subset = subset[subset['Approach'] == approach]
        subset = subset[subset['Size'].isin(sizes)]
        plt.plot(subset['Size'], subset['AdaptTime'], marker='o', label=approach, linewidth=2, markersize=8)

    plt.xlabel('Maze Size')
    plt.ylabel('Adaptation Time (s)')
    plt.title(f'Adaptation Time vs. Maze Size ({difficulty} Difficulty)')
    plt.xticks(sizes)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'adapt_time_line_{difficulty.lower()}.png')
    plt.close()

# 2. Line Plots: Success Rate vs. Number of Changes for Each Difficulty (300x300)
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Size'] == 300) & (data['Difficulty'] == difficulty)]
        plt.plot(subset[subset['Approach'] == approach]['Changes'],
                 subset[subset['Approach'] == approach]['SuccessRate'] * 100,
                 label=approach, marker='o', linewidth=2, markersize=8)
    plt.xlabel('Number of Changes')
    plt.ylabel('Success Rate (%)')
    plt.title(f'Success Rate vs. Changes (300x300, {difficulty} Difficulty)')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'success_rate_line_{difficulty.lower()}.png')
    plt.close()

# 3. Heatmap: Average Path Length (unchanged, single plot)
plt.figure(figsize=(10, 6))
for approach in data['Approach'].unique():
    subset = data[(data['Changes'] > 0)].groupby(['Size', 'Difficulty'])['AvgPathLength'].mean().unstack()
    plt.subplot(1, 3, list(data['Approach'].unique()).index(approach) + 1)
    sns.heatmap(subset, annot=True, cmap='YlOrRd', fmt='.2f')
    plt.title(f'{approach} Avg Path Length')
plt.tight_layout()
plt.savefig('avg_path_length_heatmap.png')
plt.close()

# 4. Stacked Bar Charts: Cumulative Time for Each Difficulty (Max Changes)
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in data['Approach'].unique():
        subset = data[(data['Difficulty'] == difficulty) & (data['Changes'] == data[data['Difficulty'] == difficulty]['Changes'].max())]
        subset = subset[subset['Size'].isin(sizes)]
        initial = subset[subset['Approach'] == approach]['InitialTime']
        adapt = subset[subset['Approach'] == approach]['AdaptTime']
        plt.bar(subset[subset['Approach'] == approach]['Size'], initial, label=f'{approach} Initial', alpha=0.7)
        plt.bar(subset[subset['Approach'] == approach]['Size'], adapt, bottom=initial, label=f'{approach} Adapt', alpha=0.7)
    plt.xlabel('Maze Size')
    plt.ylabel('Total Time (s)')
    plt.title(f'Cumulative Time ({difficulty} Difficulty, Max Changes)')
    plt.xticks(sizes)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'cumulative_time_stacked_{difficulty.lower()}.png')
    plt.close()

# 5. Scatter Plot: Success Rate vs. Adaptation Time (unchanged, single plot)
plt.figure(figsize=(12, 6))
for approach in data['Approach'].unique():
    subset = data[(data['Changes'] == 20) & (data['Approach'] == approach)]
    plt.scatter(subset['AdaptTime'],
                subset['SuccessRate'] * 100,
                s=subset['Size'] * 10, label=approach, alpha=0.5)
plt.xlabel('Adaptation Time (s)')
plt.ylabel('Success Rate (%)')
plt.title('Success Rate vs. Adaptation Time (20 Changes)')
plt.legend()
plt.savefig('success_vs_time_scatter.png')
plt.close()