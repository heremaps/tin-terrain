# Zemlya

Like Terra, the "Zemlya" meshing algorithm implemented in the TIN Terrain tool is also a greedy insertion algorithm, but with ideas borrowed from cartographic maps, particularly the quad-tree data structure commonly used in web maps.

In general, Zemlya offers similar performance and slightly better mesh qualities (given the same number of vertices/triangles) as Terra, especially for DTM (Digital Terrain Model) datasets.

With the `tin-terrain` command line tool, you can pass in an option `--method zemlya` to use Zemlya as the mesh generation algorithm.

## Hierarchical Greedy Insertion

The idea of "Zemlya" came from the quad-tree data structure often used in web maps. In web maps, different zoom levels are used to display maps at different levels of details. When you look at a terrain map, at lower zoom levels you only see big mountain ranges, but as you zoom into the map, you start to see more and more detailed local terrain features.

The key insight is that resamping the original heightmap at different resolutions can reveal terrain structures at corresponding scales. By resampling at different zoom levels, we can fix the Greedy Insertion algorithm’s biggest shortcoming: its heavy dependency on the points from the original raster.

Zemlya is a hierarchical greedy insertion algorithm. The algorithm works as follows:

1. First, it repeatedly downsamples the original raster to create average values for lower zoom levels, all the way to zoom level 1.
2. Then it iterates over all the zoom levels in a top-down fashion, starting with zoom level 1. At each zoom level, we start with the mesh generated from the previous zoom level, then insert the average values at this zoom level using the same greedy insertion algorithm as in Terra.
3. We iterate over all the zoom levels until we reach the final zoom level, which consists of points from the original raster.

If we only run greedy insertion on the final zoom level, Zemlya is equivalent to Terra. However, by taking advantage of average values from previous levels and gradually refining the terrain meshes, we can achieve better mesh qualities using fewer vertices/triangles, since mesh approximations from previous zoom levels provide a much better initial configuration than the simplistic initial mesh used in Terra.

Compared to the Greedy Insertion algorithm:

* We have to scan more points since we iterate over all the lower zoom levels as well. For 1M points, N = 1000, number of zoom levels = 10, so Zemlya need to scan around 1.33M points, about 33% more than Terra.
* However, since Zemlya always starts with a pretty good mesh approximation at each zoom level, the required amount of adjustment becomes significantly less. This means fewer triangles/vertices will be actually inserted into the final mesh.
* Zemlya uses not only point values from the original raster, but also many levels of average values, so it’s not as heavily dependent on the data points in the original raster as Terra.

## Implementation

In the TIN Terrain tool, the Zemlya meshing algorithm is implemented in "ZemlyaMeshing.h/cpp".

The key method is `ZemlyaMesh::greedy_insert(double max_error)`. It takes a single input parameter which is the vertical error tolerance in meters. This input parameter should be set to a value comparable to or larger than the spatial resolution of the original raster, since the meshing algorithm will not be able to produce a mesh with a higher resolution than in the original raster.

In the `greedy_insert()` method implementation, we first create another raster `m_sample` to store average values, with the same dimensions as the original raster.

```c
int w = m_raster->get_width();
int h = m_raster->get_height();
m_sample.allocate(w, h);
```

We also determine the max zoom level, which is the level where the original raster lies. Since we downsample every four pixels into one pixel as we move down a zoom level, the max level can be calculated by `log2(max(width, height)`.

```c
m_max_level = static_cast<int>(ceil(log2(w > h ? w : h)));
```

Then we calculate the average values by repeatedly downsampling the original raster points. At zoom level `m_max_level - 1`, we average over the values from the original raster, which are stored in `m_raster`. As we go down one more level, we average over the values from the previous level `m_max_level - 1`, which are stored in `m_sample`. We continue this downsampling process until we finish level 1.

Note that we have to take special caution when dealing with missing values. We devised a special averaging function which essentially ignores missing values from input points, and returns the average value of only non-missing input points. If all input points to the averaging function are missing, the averaging function returns `NAN`.

```c
// Downsampling
for(int level = m_max_level - 1; level >= 1; level--)
{
    int step = m_max_level - level;
    for(int y = 0; y < h; y += pow(2, step))
    {
        for(int x = 0; x < w; x += pow(2, step))
        {
            if(step == 1)
            {
                double v1 = y < h && x < w ? m_raster->value(y, x) : NAN;
                double v2 = y < h && x + 1 < w ? m_raster->value(y, x + 1) : NAN;
                double v3 = y + 1 < h && x < w ? m_raster->value(y + 1, x) : NAN;
                double v4 = y + 1 < h && x + 1 < w ? m_raster->value(y + 1, x + 1) : NAN;

                if(y + 1 < h && x + 1 < w)
                {
                    m_sample.value(y + 1, x + 1) = average_of(v1, v2, v3, v4, no_data_value);
                }
            }
            else
            {
                int co = pow(2, step - 1); // offset
                int d = pow(2, step - 2); // delta

                double v1 = y + co - d < h && x + co - d < w
                    ? m_sample.value(y + co - d, x + co - d)
                    : NAN;
                double v2 = y + co - d < h && x + co + d < w
                    ? m_sample.value(y + co - d, x + co + d)
                    : NAN;
                double v3 = y + co + d < h && x + co - d < w
                    ? m_sample.value(y + co + d, x + co - d)
                    : NAN;
                double v4 = y + co + d < h && x + co + d < w
                    ? m_sample.value(y + co + d, x + co + d)
                    : NAN;

                if(y + co < h && x + co < w)
                {
                    m_sample.value(y + co, x + co) =
                        average_of(v1, v2, v3, v4, no_data_value);
                }
            }
        }
    }
}
```

We also need to take care of points lying outside of the raster bounds. Those out-of-bounds values are simply treated as `NAN` in the averaging process.

Then we start with zoom level 1, and use an initial configuration of two triangles formed by the four corner points of the original raster. This is the same as in Terra.

```c
// Ensure the four corners are not NAN, otherwise the algorithm can't proceed.
this->repair_point(0, 0);
this->repair_point(0, h - 1);
this->repair_point(w - 1, h - 1);
this->repair_point(w - 1, 0);

// Initialize the mesh to two triangles with the height field grid corners as vertices
this->init_mesh(glm::dvec2(0, 0), glm::dvec2(0, h - 1), glm::dvec2(w - 1, h - 1), glm::dvec2(w - 1, 0));
```

We also create three more temporary rasters: `m_result` is used to store vertices in the final mesh, `m_insert` is used to store points to insert at each zoom level, and `m_used` is used to mark which point is already inserted so it won't be inserted again at this zoom level.

Then we iterate over the zoom levels.

```c
for(int level = 1; level <= m_max_level; level++)
{
  // 1. figure out which points may be inserted at this zoom level
  // 2. insert the points using greedy insertion
}
```

There are two crucial steps we perform at each zoom level:
1. Figure out which points we may insert at this zoom level
2. Insert the points using the greedy insertion algorithm (same as in Terra)

Which points should we insert at a particular zoom level? We need to insert average points in `m_sample` for this zoom level, but we also need to update points from previous levels by shrinking their commanding areas. This is crucial because average values from lower zoom levels represent values from bigger areas, but all points we insert at a zoom level must have the same size of commanding areas, or put another way, every downsampled pixel should have the same resolution at a particular zoom level. These updated points from previous levels are also added to `m_insert` so they also participate in the greedy insertion process.

This is how we update the points from previous levels:

```c
double d = pow(2, step - 3); // delta

for(int y = 0; y < h; y++)
{
    for(int x = 0; x < w; x++)
    {
        double& z = m_insert.value(y, x);
        if(terra::is_no_data(z, no_data_value))
        {
            continue;
        }

        const double v1 =
            y - d < h && x - d < w ? m_sample.value(y - d, x - d) : NAN;
        const double v2 =
            y - d < h && x + d < w ? m_sample.value(y - d, x + d) : NAN;
        const double v3 =
            y + d < h && x - d < w ? m_sample.value(y + d, x - d) : NAN;
        const double v4 =
            y + d < h && x + d < w ? m_sample.value(y + d, x + d) : NAN;
        const double avg = average_of(v1, v2, v3, v4, no_data_value);
        if(terra::is_no_data(avg, no_data_value))
        {
            continue;
        }
        z = avg;
    }
}
```

After we figure out which points to insert at a zoom level, we insert them one by one using the same greedy insertion algorithm as used in Terra.

```c
// Iterate until the error threshold is met
while(!m_candidates.empty())
{
    terra::Candidate candidate = m_candidates.grab_greatest();

    if(candidate.importance < m_max_error) continue;

    // Skip if the candidate is not the latest
    if(m_token.value(candidate.y, candidate.x) != candidate.token) continue;

    m_result.value(candidate.y, candidate.x) = candidate.z;
    m_used.value(candidate.y, candidate.x) = 1;

    this->insert(glm::dvec2(candidate.x, candidate.y), candidate.triangle);
}
```

Finally, after the iteration process is over, we can export the mesh to an OBJ file using the `ZemlyaMesh::convert_to_mesh()` method.

## Related Research

After we implemented the Zemlya meshing algorithm, we found out that this idea was already discovered and explored in at least one paper:

[Zheng, Xianwei, et al. "A VIRTUAL GLOBE-BASED MULTI-RESOLUTION TIN SURFACE MODELING AND VISUALIZETION METHOD." International Archives of the Photogrammetry, Remote Sensing & Spatial Information Sciences 41 (2016).](https://www.int-arch-photogramm-remote-sens-spatial-inf-sci.net/XLI-B2/459/2016/isprs-archives-XLI-B2-459-2016.pdf)

This paper describes a very similar hierarchical enhancement to the Terra meshing algorithm, although they opted to adjust the "max error" input parameter to achieve different meshing resolutions instead of precomputing a pyramid of average values from the original raster. These two approaches are equivalent in principle.
