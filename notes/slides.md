---
title: Accelerating Volumetric Ray Tracing using SIMD Instructions
author: Sebastian Dawid (sdawid@techfak.uni-bielefeld.de)
---

Agenda
===

* Algorithm
    * Overview
    * Maths
* Optimization
    * SIMD
    * Tiling
* Preliminary Results

<!--end_slide-->

Algorithm
===

# Ray Tracing

![](./ray-tracing.png)

<!--end_slide-->

Algorithm
===

# Mathematical Representation

Integrate Radiance for each pixel in the image via approximation:
```latex +render
\begin{align}
    \hat{L}(\mathbf{o}, \mathbf{n}) &= \sum_{q = 0}^{N - 1} \mathbf{a}_q \sum_{s \in S_q} \lambda_q T(\mathbf{o}, \mathbf{n}, s)G_q(\mathbf{o} + s\mathbf{n})\\
    T(\mathbf{o}, \mathbf{n}, s) &= \exp{\left( \sum_{q = 0}^{N - 1} \frac{\overbar{\sigma}_q\overbar{c}_q}{\sqrt{\frac{2}{\pi}}} \left( \text{erf}{\left( \frac{- \overbar{\mu}_q}{\sqrt{2}\overbar{\sigma}_q} \right)} - \text{erf}{\left( \frac{s - \overbar{\mu}_q}{\sqrt{2}\overbar{\sigma}_q} \right)} \right) \right)}\\
    \overbar{\mu}_q &= (\mathbf{\mu}_q - \mathbf{o})^T\mathbf{n},\quad \overbar{\sigma}_q = \sigma_q\\
    \overbar{c}_q &= c \exp{\left( -\frac{(\mathbf{\mu}_q - \mathbf{o})^T(\mathbf{\mu}_q - \mathbf{o}) - \overbar{\mu}_q}{2\overbar{\sigma}_q} \right)}
\end{align}
```
```typst +render
- $hat(L)$: Radiance
- $bold(o)$: origin of the ray
- $bold(n)$: direction of the ray
- $T$: transmittance
- $G_q$: density of the gaussian $q$
- $bold(a)_q$: albedo of gaussian $q$
- $S_q$: set of sample points along the ray to consider for gaussian $q$
- $lambda_q$: step length for gaussian $q$
- $N$: the number of gaussians
```
<!--end_slide-->

Optimization
===

# SIMD

Modern CPUs SIMD (Single Instruction Multiple Data) instruction sets allow us to perform a given operation on a vector values at the same time without distributing the tasks to multiple CPUs/threads.

<!--pause-->

## Some Notation

```latex +render
\begin{itemize}
\item $X^W$ - SIMD vector of width $W$
\item $\odot$ - Element-wise multiplication of SIMD vectors
\item $\langle \cdot \rangle^W$ - Broadcast a scalar to a SIMD vector of width $W$
\end{itemize}
```

<!--end_slide-->

Optimization
===

# SIMD

Now we can rewrite the radiance formular to operate on multiple gaussians at the same time:
```latex +render
\begin{align}
    T^W(\mathbf{o}, \mathbf{n}, s) &= \exp{\left( \sum_{m = 0}^{\lceil\frac{N}{W}\rceil - 1} \begin{pmatrix}
        \frac{\overbar{\sigma}_{mW}\overbar{c}_{mW}}{\sqrt{\frac{2}{\pi}}}\\
        \vdots\\
        \frac{\overbar{\sigma}_{(m + 1)W - 1}\overbar{c}_{(m + 1)W - 1}}{\sqrt{\frac{2}{\pi}}}\\
    \end{pmatrix} \odot \begin{pmatrix}
        \text{erf}{\left( \frac{- \overbar{\mu}_{mW}}{\sqrt{2}\overbar{\sigma}_{mW}} \right)} - \text{erf}{\left( \frac{s - \overbar{\mu}_{mW}}{\sqrt{2}\overbar{\sigma}_{mW}} \right)}\\
        \vdots\\
        \text{erf}{\left( \frac{- \overbar{\mu}_{(m + 1)W - 1}}{\sqrt{2}\overbar{\sigma}_{(m + 1)W - 1}} \right)} - \text{erf}{\left( \frac{s - \overbar{\mu}_{(m + 1)W - 1}}{\sqrt{2}\overbar{\sigma}_{(m + 1)W - 1}} \right)}
    \end{pmatrix} \right)}\\
    \hat{L}(\mathbf{o}, \mathbf{n}) &= \sum_{q = 0}^{N - 1} \left(\mathbf{a}_q \sum_{s \in S_q} \lambda_q \sum_{i = 1}^W \left( T^W_i(\mathbf{o}, \mathbf{n}, s) \right) G_q(\mathbf{o} + s\mathbf{n}) \right)\\
\end{align}
```

<!--pause-->

Or we can perform the calculation for multiple pixels at the same time:
```latex +render
\[
    \hat{L}^W(\mathbf{o}^W, \mathbf{n}^W) = \sum_q \left( \langle\mathbf{a}_q\rangle^W \odot \sum_{s \in S_q} \left( \langle \lambda_q\rangle^W \odot T(\mathbf{o}^W, \mathbf{n}^W,\ \langle s\rangle^W) \odot G_q(\mathbf{o}^W + \langle s\rangle^W \odot \mathbf{n}^W) \right) \right)
\]
```
<!--end_slide-->

Optimization
===

# Tiling

- Seperate the image into tiles of some size.
- Determine which gaussians affect which tiles.
- Only consider gaussians that have an effect on a tile when rendering that tile.

<!--pause-->

```typst +render
#import "@preview/cetz:0.2.2"
#set align(center)
#cetz.canvas(length: 5cm, {
    import cetz.draw: *

    circle((0.3, 0.15), radius: 0.01, fill: red, stroke: red)
    circle((0.3, 0.15), radius: 0.2, stroke: red)
    circle((0.4, 0.3), radius: 0.01, fill: green, stroke: green)
    circle((0.4, 0.3), radius: 0.15, stroke: green)
    circle((0.5, 0.2), radius: 0.01, fill: blue, stroke: blue)
    circle((0.5, 0.2), radius: 0.1, stroke: blue)

    circle((1.3, 0.6), radius: 0.01, fill: purple, stroke: purple)
    circle((1.3, 0.6), radius: 0.275, stroke: purple)

    rect((0,0), (rel: (1.6, 0.9)), name: "image")
    grid((0,0), (rel: (1.6, 0.9)), stroke: gray + .5pt, step: (0.4, 0.225))
})
```

<!--end_slide-->

Preliminary Results
===

# Standford Teapot
Interpreting the 3644 vertices of the standford teapot model as gaussians yields:

![](./teapot.png)

<!--end_slide-->

Preliminary Results
===

# Timings
## Non Tiled
| Method | Time |
| :-- | :-- |
| Sequential           | >30 min |
| SIMD along gaussians | >30 min |
| SIMD along pixels    | >30 min |

## Tiled
| Method | Time |
| :-- | :-- |
| Sequential               | 1810664.6 ms |
| SIMD along gaussians     | 343416.84 ms |
| SIMD along pixels        | 310050.78 ms |
| Threads                  | 400987.12 ms |
| SIMD gaussians + Threads |  73612.66 ms |
| SIMD pixels + Threads    |  66775.25 ms |
