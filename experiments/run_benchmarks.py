#!/usr/bin/env python3

import csv
import json
import subprocess
import tempfile
from copy import deepcopy
from pathlib import Path

kRoot = Path(__file__).resolve().parent.parent
kExe = kRoot / "build" / "diplodocus"
kTemplate = kRoot / "experiments" / "config_template.json"
kRunsDir = kRoot / "experiments" / "runs"
kResultsDir = kRoot / "experiments" / "results"


kScenePaths = [
    "Dragon/Dragon",
    "Hairball/Hairball",
    "PowerPlant/PowerPlant",
    "Rungholt/Rungholt",
    "Skydemo/skydemo",
    "Sponza/sponza",
    "Stegosaurus/Stegosaurus",
    "Venlo/Venlo",
]

kCameras = ["cam0", "cam1", "cam2"]

kVariants = [
    "ploc",
    "ploc_emc1",
    "ploc_emc2",
    "ploc_sobb",
    "ploc_emc1_sobb",
    "ploc_emc2_sobb",
]

kRepeats = 5
kSeed = 42
kTotalRuns = len(kScenePaths) * len(kCameras) * len(kVariants) * kRepeats


def load_template() -> dict:
    with kTemplate.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def build_config(
    template: dict,
    scene_path: str,
    meta_data: str,
    accel_type: str,
    seed: int,
    image_path: Path,
    stats_path: Path,
) -> dict:
    config = deepcopy(template)
    config["scene_load"]["name"] = scene_path
    config["scene_load"]["meta_data"] = meta_data
    config["render"]["seed"] = seed
    config["acceleration_structure"]["acceleration_structure_type"] = accel_type
    config["image_export"]["filepath"] = str(image_path)
    config["stats_export"]["filepath"] = str(stats_path)
    config["stats_export"]["stats_export_format"] = "json"
    return config


def main() -> int:
    if not kExe.exists():
        raise SystemExit(f"Missing executable: {kExe}")
    if not kTemplate.exists():
        raise SystemExit(f"Missing config template: {kTemplate}")

    template = load_template()
    kResultsDir.mkdir(parents=True, exist_ok=True)
    csv_path = kResultsDir / "raw_results.csv"

    fieldnames = [
        "scene",
        "camera_id",
        "repeat_id",
        "variant",
        "renderer_type",
        "width",
        "height",
        "pixel_sample_cnt",
        "area_light_sample_cnt",
        "max_depth",
        "seed",
        "nn_search_radius",
        "kdop_size",
        "max_triangles_per_leaf",
        "bin_count",
        "build_time_ms",
        "init_time_ns",
        "memcopy_time_ns",
        "morton_construction_time_ns",
        "morton_sort_time_ns",
        "nn_search_time_ns",
        "match_and_classify_time_ns",
        "prefix_scan_time_ns",
        "merge_and_compact_time_ns",
        "sobb_refit_time_ns",
        "kernel_time_ns",
        "node_count",
        "inner_node_count",
        "leaf_node_count",
        "bvh_cost",
        "memory_consumption_bytes",
        "frame_time_s",
        "raytracing_time_s",
        "primary_ray_count",
        "secondary_ray_count",
        "shadow_ray_count",
        "query_count",
        "intersection_count",
        "traversal_count",
        "stats_path",
    ]

    run_idx = 0
    with csv_path.open("w", encoding="utf-8", newline="") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()

        for scene_path in kScenePaths:
            scene_name = scene_path.split("/")[0]
            for camera_id in kCameras:
                meta_data = f"{scene_path}_{camera_id}"
                for variant_name in kVariants:
                    for repeat_id in range(kRepeats):
                        run_idx += 1
                        run_dir = (
                            kRunsDir
                            / scene_name
                            / camera_id
                            / variant_name
                            / f"repeat_{repeat_id:02d}"
                        )
                        run_dir.mkdir(parents=True, exist_ok=True)
                        stats_path = run_dir / "stats.json"
                        image_path = run_dir / "image.ppm"
                        log_path = run_dir / "run.log"

                        config = build_config(
                            template,
                            scene_path=scene_path,
                            meta_data=meta_data,
                            accel_type=variant_name,
                            seed=kSeed + repeat_id,
                            image_path=image_path,
                            stats_path=stats_path,
                        )

                        with tempfile.NamedTemporaryFile(
                            "w", suffix=".json", dir=run_dir, delete=False
                        ) as tmp:
                            json.dump(config, tmp, indent=2)
                            tmp.write("\n")
                            config_path = Path(tmp.name)

                        print(
                            f"[{run_idx:03d}/{kTotalRuns}] {scene_name} | {camera_id} | {variant_name} | repeat {repeat_id}",
                            flush=True,
                        )
                        with log_path.open("w", encoding="utf-8") as log_file:
                            subprocess.run(
                                [str(kExe), str(config_path)],
                                cwd=kRoot,
                                stdout=log_file,
                                stderr=subprocess.STDOUT,
                                check=True,
                            )
                        config_path.unlink(missing_ok=True)

                        with stats_path.open("r", encoding="utf-8") as handle:
                            run_json = json.load(handle)

                        render = run_json["config"]["render"]
                        accel = run_json["config"]["acceleration_structure"]
                        construction = run_json["stats"]["construction"]
                        raytracing = run_json["stats"]["raytracing"]

                        writer.writerow(
                            {
                                "scene": scene_name,
                                "camera_id": camera_id,
                                "repeat_id": repeat_id,
                                "variant": variant_name,
                                "renderer_type": render["renderer_type"],
                                "width": render["width"],
                                "height": render["height"],
                                "pixel_sample_cnt": render["pixel_sample_cnt"],
                                "area_light_sample_cnt": render[
                                    "area_light_sample_cnt"
                                ],
                                "max_depth": render["max_depth"],
                                "seed": render["seed"],
                                "nn_search_radius": accel["nn_search_radius"],
                                "kdop_size": accel["kdop_size"],
                                "max_triangles_per_leaf": accel[
                                    "max_triangles_per_leaf"
                                ],
                                "bin_count": accel["bin_count"],
                                "build_time_ms": construction["build_time"],
                                "init_time_ns": construction["init_time"],
                                "memcopy_time_ns": construction["memcopy_time"],
                                "morton_construction_time_ns": construction[
                                    "morton_construction_time"
                                ],
                                "morton_sort_time_ns": construction["morton_sort_time"],
                                "nn_search_time_ns": construction["nn_search_time"],
                                "match_and_classify_time_ns": construction[
                                    "match_and_classify_time"
                                ],
                                "prefix_scan_time_ns": construction["prefix_scan_time"],
                                "merge_and_compact_time_ns": construction[
                                    "merge_and_compact_time"
                                ],
                                "sobb_refit_time_ns": construction["sobb_refit_time"],
                                "kernel_time_ns": construction["kernel_time"],
                                "node_count": construction["node_count"],
                                "inner_node_count": construction["inner_node_count"],
                                "leaf_node_count": construction["leaf_node_count"],
                                "bvh_cost": construction["bvh_cost"],
                                "memory_consumption_bytes": construction[
                                    "memory_consumption"
                                ],
                                "frame_time_s": raytracing["frame_time"],
                                "raytracing_time_s": raytracing["raytracing_time"],
                                "primary_ray_count": raytracing["primary_ray_count"],
                                "secondary_ray_count": raytracing[
                                    "secondary_ray_count"
                                ],
                                "shadow_ray_count": raytracing["shadow_ray_count"],
                                "query_count": raytracing["query_count"],
                                "intersection_count": raytracing["intersection_count"],
                                "traversal_count": raytracing["traversal_count"],
                                "stats_path": str(stats_path),
                            }
                        )
                        csv_file.flush()

    print(f"Wrote {run_idx} rows to {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
