import pandas as pd
import matplotlib.pyplot as plt
import glob
import re
import os

DATA_DIR = "data"
PLOTS_DIR = "plots"
os.makedirs(PLOTS_DIR, exist_ok=True)

# ---------- Plot 1: flood ratio & hit rate over time, single run ----------
def plot_single_run_trend(stats_path, out_name, title):
    df = pd.read_csv(stats_path)
    fig, ax = plt.subplots(figsize=(9, 5))
    ax.plot(df["bucket_index"], df["flood_ratio"], label="Flood ratio", marker="o")
    ax.plot(df["bucket_index"], df["hit_rate"], label="Table hit rate", marker="s")
    ax.set_xlabel("Time bucket")
    ax.set_ylabel("Ratio")
    ax.set_ylim(-0.05, 1.05)
    ax.set_title(title)
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, out_name), dpi=150)
    plt.close(fig)
    print(f"wrote plots/{out_name}")

# ---------- Plot 2: flood ratio & hit rate vs locality (the headline result) ----------
def plot_locality_sweep():
    pattern = os.path.join(DATA_DIR, "stats_locality_*.csv")
    files = sorted(glob.glob(pattern))
    if not files:
        print("No locality sweep files found — run Step 1 first.")
        return

    localities = []
    avg_flood_ratios = []
    avg_hit_rates = []

    for f in files:
        match = re.search(r"stats_locality_([\d.]+)\.csv", f)
        locality = float(match.group(1))
        df = pd.read_csv(f)
        # weight by frame_count so buckets with more frames count proportionally more
        total_frames = df["frame_count"].sum()
        avg_flood = (df["flood_ratio"] * df["frame_count"]).sum() / total_frames
        # hit_rate excludes broadcast-only buckets from its own denominator already;
        # weight by (hits + floods_unknown) rather than frame_count to stay consistent
        unicast_attempts = df["hits"] + df["floods_unknown"]
        total_unicast = unicast_attempts.sum()
        avg_hit = (df["hit_rate"] * unicast_attempts).sum() / total_unicast if total_unicast > 0 else 0

        localities.append(locality)
        avg_flood_ratios.append(avg_flood)
        avg_hit_rates.append(avg_hit)

    order = sorted(range(len(localities)), key=lambda i: localities[i])
    localities = [localities[i] for i in order]
    avg_flood_ratios = [avg_flood_ratios[i] for i in order]
    avg_hit_rates = [avg_hit_rates[i] for i in order]

    fig, ax = plt.subplots(figsize=(9, 5))
    ax.plot(localities, avg_flood_ratios, label="Flood ratio", marker="o")
    ax.plot(localities, avg_hit_rates, label="Table hit rate", marker="s")
    ax.set_xlabel("Traffic locality parameter")
    ax.set_ylabel("Ratio")
    ax.set_ylim(-0.05, 1.05)
    ax.set_title("Flood ratio vs. hit rate as traffic locality changes")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "flood_vs_hit_by_locality.png"), dpi=150)
    plt.close(fig)
    print("wrote plots/flood_vs_hit_by_locality.png")

# ---------- Plot 3: MAC table size over time with eviction markers ----------
def plot_table_evolution(evo_path, out_name, title):
    df = pd.read_csv(evo_path)
    fig, ax = plt.subplots(figsize=(9, 5))
    ax.plot(df["bucket_index"], df["table_size"], label="Table size", color="tab:blue")

    evicted_mask = df["evicted_count"] > 0
    ax.scatter(df.loc[evicted_mask, "bucket_index"], df.loc[evicted_mask, "table_size"],
               color="tab:red", zorder=5, label="Eviction occurred", marker="x")

    ax.set_xlabel("Time bucket")
    ax.set_ylabel("MAC table size (entries)")
    ax.set_title(title)
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, out_name), dpi=150)
    plt.close(fig)
    print(f"wrote plots/{out_name}")


if __name__ == "__main__":
    plot_single_run_trend("data/stats_synthetic.csv", "synthetic_trend.png",
                          "Synthetic run: flood ratio & hit rate over time")
    plot_single_run_trend("data/stats_trace.csv", "trace_trend.png",
                          "Real trace: flood ratio & hit rate over time")
    plot_locality_sweep()
    plot_table_evolution("data/table_evolution_synthetic.csv", "table_evolution_synthetic.png",
                         "MAC table evolution (synthetic run)")
    plot_table_evolution("data/table_evolution_trace.csv", "table_evolution_trace.png",
                         "MAC table evolution (real trace)")