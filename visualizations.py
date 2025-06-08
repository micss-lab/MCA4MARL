import pandas as pd
import matplotlib.pyplot as plt
import os

# Create 'plots' directory if it doesn't exist
plots_dir = "plots_incremental_smart"
if not os.path.exists(plots_dir):
    os.makedirs(plots_dir)

# Load data
agg_data = pd.read_csv("results_incremental_smart.csv")
detailed_data = pd.read_csv("results_incremental_detailed_smart.csv")

# Define difficulties and sizes
difficulties = ["Easy", "Medium", "Hard"]
sizes = [20, 50, 100, 200, 300, 400]

# 1. Line Plots: Adaptation Time vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in agg_data["Approach"].unique():
        subset = agg_data[
            (agg_data["Difficulty"] == difficulty) & (agg_data["Approach"] == approach)
        ].sort_values("Size")
        if not subset.empty:
            plt.plot(
                subset["Size"],
                subset["AdaptTimePerStep"],
                marker="o",
                label=approach,
                linewidth=2,
                markersize=8,
            )
    plt.xlabel("Maze Size")
    plt.ylabel("Adaptation Time per Step (s)")
    plt.title(
        f"Adaptation Time per Step vs. Maze Size ({difficulty} Difficulty, 50 Steps)"
    )
    plt.xticks(sizes)
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/adapt_time_line_{difficulty.lower()}.png")
    plt.close()

# 2. Line Plots: Success Rate vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in agg_data["Approach"].unique():
        subset = agg_data[
            (agg_data["Difficulty"] == difficulty) & (agg_data["Approach"] == approach)
        ].sort_values("Size")
        if not subset.empty:
            plt.plot(
                subset["Size"],
                subset["AvgSuccessRate"] * 100,
                marker="o",
                label=approach,
                linewidth=2,
                markersize=8,
            )
    plt.xlabel("Maze Size")
    plt.ylabel("Average Success Rate (%)")
    plt.title(f"Avg Success Rate vs. Maze Size ({difficulty} Difficulty, 50 Steps)")
    plt.xticks(sizes)
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/success_rate_line_{difficulty.lower()}.png")
    plt.close()

# 3. Line Plots: Avg Path Length vs. Maze Size for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(10, 6))
    for approach in agg_data["Approach"].unique():
        subset = agg_data[
            (agg_data["Difficulty"] == difficulty) & (agg_data["Approach"] == approach)
        ].sort_values("Size")
        if not subset.empty:
            plt.plot(
                subset["Size"],
                subset["AvgPathLength"],
                marker="o",
                label=approach,
                linewidth=2,
                markersize=8,
            )
    plt.xlabel("Maze Size")
    plt.ylabel("Average Path Length")
    plt.title(f"Avg Path Length vs. Maze Size ({difficulty} Difficulty, 50 Steps)")
    plt.xticks(sizes)
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/avg_path_length_line_{difficulty.lower()}.png")
    plt.close()

# 4. Scatter Plots: Success Rate vs. Adaptation Time for Each Difficulty
for difficulty in difficulties:
    plt.figure(figsize=(12, 6))
    for approach in agg_data["Approach"].unique():
        subset = agg_data[
            (agg_data["Difficulty"] == difficulty) & (agg_data["Approach"] == approach)
        ]
        if not subset.empty:
            plt.scatter(
                subset["AdaptTimePerStep"],
                subset["AvgSuccessRate"] * 100,
                s=subset["Size"] * 5,
                label=approach,
                alpha=0.5,
            )
    plt.xlabel("Adaptation Time per Step (s)")
    plt.ylabel("Average Success Rate (%)")
    plt.title(
        f"Avg Success Rate vs. Adaptation Time per Step ({difficulty} Difficulty, 50 Steps)"
    )
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{plots_dir}/success_vs_time_scatter_{difficulty.lower()}.png")
    plt.close()

# 5. Scatter Plot: Adaptation Time vs. Number of Changes per Step for Each Size and Difficulty
for size in sizes:
    for difficulty in difficulties:
        plt.figure(figsize=(12, 6))
        for approach in detailed_data["Approach"].unique():
            subset = detailed_data[
                (detailed_data["Size"] == size)
                & (detailed_data["Difficulty"] == difficulty)
                & (detailed_data["Approach"] == approach)
                & (detailed_data["TimeStep"] > 0)
            ]
            if not subset.empty:
                plt.scatter(
                    subset["NumChanges"],
                    subset["AdaptTime"],
                    s=50,
                    label=approach,
                    alpha=0.5,
                )
        plt.xlabel("Number of Changes per Step")
        plt.ylabel("Adaptation Time (s)")
        plt.title(
            f"Adaptation Time vs. Num Changes per Step ({size}x{size}, {difficulty} Difficulty)"
        )
        plt.xticks(range(1, 11))
        plt.grid(axis="y", linestyle="--", alpha=0.7)
        plt.legend()
        plt.tight_layout()
        plt.savefig(
            f"{plots_dir}/adapt_time_vs_changes_{size}x{size}_{difficulty.lower()}.png"
        )
        plt.close()

# 6. Line Plot: Cumulative Adaptation Time vs. Time Step for Each Size and Difficulty
for size in sizes:
    for difficulty in difficulties:
        plt.figure(figsize=(12, 6))
        for approach in detailed_data["Approach"].unique():
            subset = detailed_data[
                (detailed_data["Size"] == size)
                & (detailed_data["Difficulty"] == difficulty)
                & (detailed_data["Approach"] == approach)
            ].sort_values("TimeStep")
            if not subset.empty:
                cumulative_time = subset["AdaptTime"].cumsum()
                plt.plot(
                    subset["TimeStep"], cumulative_time, label=approach, linewidth=2
                )
        plt.xlabel("Time Step")
        plt.ylabel("Cumulative Adaptation Time (s)")
        plt.title(
            f"Cumulative Adaptation Time vs. Time Step ({size}x{size}, {difficulty} Difficulty)"
        )
        plt.grid(axis="y", linestyle="--", alpha=0.7)
        plt.legend()
        plt.tight_layout()
        plt.savefig(
            f"{plots_dir}/cumulative_adapt_time_{size}x{size}_{difficulty.lower()}.png"
        )
        plt.close()

# 7. Line Plot: Success Rate vs. Time Step for Each Size and Difficulty
for size in sizes:
    for difficulty in difficulties:
        plt.figure(figsize=(12, 6))
        for approach in detailed_data["Approach"].unique():
            subset = detailed_data[
                (detailed_data["Size"] == size)
                & (detailed_data["Difficulty"] == difficulty)
                & (detailed_data["Approach"] == approach)
            ].sort_values("TimeStep")
            if not subset.empty:
                plt.plot(
                    subset["TimeStep"],
                    subset["SuccessRate"] * 100,
                    label=approach,
                    linewidth=2,
                )
        plt.xlabel("Time Step")
        plt.ylabel("Success Rate (%)")
        plt.title(
            f"Success Rate vs. Time Step ({size}x{size}, {difficulty} Difficulty)"
        )
        plt.grid(axis="y", linestyle="--", alpha=0.7)
        plt.legend()
        plt.tight_layout()
        plt.savefig(
            f"{plots_dir}/success_rate_vs_timestep_{size}x{size}_{difficulty.lower()}.png"
        )
        plt.close()

# 8. Box Plot: Adaptation Time by Approach for Each Size and Difficulty
for size in sizes:
    for difficulty in difficulties:
        plt.figure(figsize=(10, 6))
        subset = detailed_data[
            (detailed_data["Size"] == size)
            & (detailed_data["Difficulty"] == difficulty)
            & (detailed_data["TimeStep"] > 0)
        ]  # Exclude initial step
        if not subset.empty:
            plt.boxplot(
                [
                    subset[subset["Approach"] == approach]["AdaptTime"]
                    for approach in detailed_data["Approach"].unique()
                ],
                tick_labels=detailed_data["Approach"].unique(),
            )
        plt.xlabel("Approach")
        plt.ylabel("Adaptation Time per Step (s)")
        plt.title(
            f"Adaptation Time Distribution by Approach ({size}x{size}, {difficulty} Difficulty)"
        )
        plt.grid(axis="y", linestyle="--", alpha=0.7)
        plt.tight_layout()
        plt.savefig(
            f"{plots_dir}/adapt_time_box_{size}x{size}_{difficulty.lower()}.png"
        )
        plt.close()
