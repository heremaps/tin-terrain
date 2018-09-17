# Mesh Generation Algorithms

Surface mesh models are commonly used in computer graphics, mechanical engineering and other fields for both visualization and simulation. There are generally two broad categories of such surface mesh models: a Triangulated Regular Network (TRN), and a Triangulated Irregular Network (TIN).

A Triangulated Regular Network (TRN) simply connects input raster points into a uniform grid of triangles. The resulting surface mesh uses the same number of triangles in each unit area, so they are typically quite dense thus unsuitable for data visualization.

A Triangulated Irregular Network (TIN) takes advantage of the fact that natural terrains often consist of slowly-changing areas that can be modelled with fewer number of triangles per unit area. The resulting mesh uses significantly fewer triangles compared to a TRN, thus is more suitable for both data storage and visualization. The amount of information loss can be controlled by a given meshing parameter (tolerance of the vertical error), thus different levels of compression can be used depending on the user's requirement.

![BCF6BEB84B3CEAA40839170C4E5ACF22.jpg](https://raw.githubusercontent.com/heremaps/tin-terrain/algorithm-docs.ylian/docs/resources/BCF6BEB84B3CEAA40839170C4E5ACF22.jpg) (Figures are from: Garland, Michael, and Paul S. Heckbert. "Fast polygonal approximation of terrains and height fields." (1995).)

## The TIN Model

The Triangulated Irregular Network (TIN) surface model consists of irregularly spaced points connected into various sizes of faces (triangles) that approximate the original terrain surface. Fewers points or triangles are used in smooth areas and more are used in rough areas. The TIN model is currently used in numerous GIS applications.

A good TIN model should satisfy the following properties:
* It should give a good approximation of the original terrain surface in both smooth and rough areas. The error map of the TIN model should not contain noticable features from the original terrain surface.
* The difference between the TIN model and the original terrain surface should be predictable and controllable, meaning that the error metric of the TIN model should go down in a predictable manner as more points or triangles are used.
* The TIN model generation should be fast.

Mathematically speaking, creating a TIN model is a 2D surface approximation problem. The ideal surface most likely would not consists of only the points from the original raster, but slightly above or under the original raster points. However, a 2D surface approximation algorithm such as Poisson Surface Reconstruction is usually very computation-intensive. That's why most practical TIN generation algorithms used today use points from the original raster and subsequently reduce the complexity of the problem to picking points then applying Delaunay triangulation.

## Algorithms for TIN Generation

There are many classes of TIN generation algorithms, but they can be roughly categorized into the following groups:
* One pass feature methods, which pick "feature" points (such as peaks, pits, ridges, and valleys) in one pass and use them as the vertex set for Delaunay triangulation.
* Multi-pass refinement methods, which start with a minimal approximation and use multiple passes to insert points from the original raster, and retriangulate at every step.
* Multi-pass decimation methods, which start with a regular mesh, then iteratively delete vertices from the mesh.

### One-pass feature methods

Various one-pass feature methods have been developed in the past few decades. We are not going to discuss these methods in detail, but we list below the most important methods and their papers.

1. Fowler and Little Algorithm:
    Fowler, Robert J., and James J. Little. "Automatic extraction of irregular network digital terrain models." ACM SIGGRAPH Computer Graphics 13.2 (1979): 199-207.
2. VIP (Very Important Points) Algorithm:
    Chen, Zi-Tan, and J. Armando Guevara. "Systematic selection of very important points (VIP) from digital terrain model for constructing triangular irregular networks." Auto-carto. Vol. 8. 1987.
3. Lee's Drop Heuristic Algorithm:
    Lee, Jay. "Comparison of existing methods for building triangular irregular network, models of terrain from grid digital elevation models." International Journal of Geographical Information System 5.3 (1991): 267-285.

### Multi-pass refinement methods

Michael Garland gave a detailed comparison of multi-pass refinement methods in his 1995 paper (Garland, Michael, and Paul S. Heckbert. "Fast polygonal approximation of terrains and height fields." (1995).) He concluded that the greedy insertion algorithm gives the best result for various types of terrains. (The most difficult case is a mix of slowly-varying terrain with fast-changing local features inside one raster.) Also, he compared using several different error metrics, and concluded that the simplest error metric (absolute error) actually works the best. In his paper he also made several optimizations to the greedy insertion algorithm to significantly improve its performance.

The greedy insertion algorithm is a refinement algorithm. It starts with a simple triangulation of the raster (typically forming two triangles with four corner points of the raster), then on each pass, finds the raster point with the highest error in the current approximation, and inserts it into the mesh. After inserting the point, we retriangulate the mesh using incremental Delaunay triangulation (meaning that only the triangles near the inserted points are updated), and then we repeat the process until no point with an error above a given threshold is left.

This algorithm is called "greedy" because each inserted point cannot be updated or removed in subsequent passes. This algorithm uses sequential insertion (insert one point at a time) instead of parallel insertion (inserting all points that exceed the error threshold at once) because parallel insertion often results in worse results since it doesn't take into account the mesh changes from inserting other points.

Michael Garland implemented his version of the Greedy Insertion algorithm in a software package called [Terra](https://mgarland.org/software/terra.html).

The Terra algorithm is implemented in the "TIN Terrain" tool as the "terra" method. This is the default meshing algorithm used by the "TIN Terrain" tool.

### Multi-pass decimation methods

We will not cover the multi-pass decimation methods here since we haven't yet looked into them. Feel free to contribute your edits to this section.

## Algorithms implemented in "TIN Terrain"

We will discuss in detail two algorithms implemented in the "TIN Terrain" tool: Terra and Zemlya. They are both greedy insertion algorithms and offer similar performance and mesh qualities. Zemlya gives noticeably smaller mesh with the same error metric especially for DTM (Digital Terrain Model) datasets.

