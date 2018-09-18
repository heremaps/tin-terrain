# Zemlya

Like Terra, the "Zemlya" meshing algorithm implemented in the TIN Terrain tool is also a greedy insertion algorithm, but with ideas borrowed from cartographic maps, particularly the quad-tree data structure used in web maps.

In general, Zemlya offers similar performance and slightly better mesh qualities (given the same number of vertices/triangles) as Terra, especially for DTM (Digital Terrain Model) datasets.

## Hierarchical Greedy Insertion

The idea of "Zemlya" came from the quad-tree data structure often used in web maps. In web maps, different zoom levels are used to display maps at different levels of details. As you zoom in to the map, you see different terrain features at different zoom levels: at low zoom levels you only see big mountain ranges, but you start to see more and more local terrain features as you zoom in.

The key insight is that resamping the original raster at different resolutions can reveal terrain structures at corresponding scales. By resampling at different zoom levels, we can fix the Greedy Insertion algorithm’s biggest shortcoming: its heavy dependency on the points in the original raster.

A hierarchical greedy insertion algorithm creates meshes at all zoom levels from level 1 to the max level (resolution of the original raster). It uses the mesh generated from the previous zoom level (`z`) as the starting configuration when running greedy insertion at zoom level `z+1`.

In more detail, Zemlya first creates average raster values at lower zoom levels (all the way to zoom level 1). We iterate over all the zoom levels in a top-down fashion, starting with zoom level 1. Average values at this zoom level are inserted using the same greedy insertion algorithm as in Terra. Then average values from zoom level 2 are inserted, and so on, until we reach the final zoom level, which consists of points from the original raster.

It we only run greedy insertion on the max level, Zemlya is equivalent to Terra. However, by taking advantage of average values from previous levels and gradually refining the terrain meshes, we can achieve better mesh qualities using fewer vertices/triangles, as  mesh approximation from the previous level provides a much better starting configuration than the simplistic initial mesh used in Terra.

Compared to the Greedy Insertion algorithm:
* We have to scan more points since we do a full scan of all triangles at each zoom level (for 1M points, N = 1000, number of zoom levels = 10, so we need to scan around 1.33 M points, compared to 1 M points in the Greedy Insertion algorithm).
* But considering the last zoom level, the Greedy Inseration algorithm starts with the four corner points as the initial configuration, but with the new algorithm, we start with a really good mesh from the previous zoom level. With the new algorithm, the required amount of adjustment in this step is significantly less.
* The new algorithm uses pixel averages from different zoom levels, so it’s not as heavily dependent on the data points in the original raster, as in the Greedy Insertion algorithm.

After we implemented the Zemlya meshing algorithm, we found out that this idea was already discovered and explored in at least one paper:

[Zheng, Xianwei, et al. "A VIRTUAL GLOBE-BASED MULTI-RESOLUTION TIN SURFACE MODELING AND VISUALIZETION METHOD." International Archives of the Photogrammetry, Remote Sensing & Spatial Information Sciences 41 (2016).](https://www.int-arch-photogramm-remote-sens-spatial-inf-sci.net/XLI-B2/459/2016/isprs-archives-XLI-B2-459-2016.pdf)

This paper describes a very similar hierarchical enhancement of the Terra meshing algorithm, although they opted to adjust the "max error" input parameter to achieve different meshing resolutions instead of calculating a pyramid of average values from the original raster. These two approaches are equivalent in principle, but it might be worthwhile comparing their results using real-life terrain datasets (not yet done).

## Implemenation

Refer to the code.

Explain how the average values are calculated.

Explain how we do insertion from top-down. How we adjust inserted points.

Use some code samples.
