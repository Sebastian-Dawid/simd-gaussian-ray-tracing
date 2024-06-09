#import "@preview/cetz:0.2.2"

#set page(width: 20cm, height: auto, margin: .5cm)

#show math.equation: block.with(fill: white, inset: 1pt)

= Integrals
$ hat(L)(o, n, gamma) = sum_q a_q sum_(s in S_q) lambda_q T(o, n, s, gamma) G_q (o + s n) $
$ T(o, n, s, gamma) = exp(-integral_0^s sum_q G_q (o + t n) dif t)\
                    = exp(sum_q frac(macron(sigma)_q macron(c)_q, sqrt(frac(2, pi))) ("erf"(frac(macron(mu)_q, sqrt(2)macron(sigma)_q)) - "erf"(frac(s - macron(mu)_q, sqrt(2) macron(mu)_q)))) $

=== Loop Structure
```c
// per pixel/ray
vec3f_t L_hat(0.0f);
for (uint64_t q = 0; q < no_gaussians; ++q) // gaussians
{
    float inner = 0.0f;
    for (int64_t k = 0; k > -5; --k) // sample density along ray
    {
        float s = mu_bar[q] + k * lambda[q];
        float T = 0.0f;
        for (...)
        {
            // approx T via erf or integral
        }
        inner += G[q](o + s*n) * T * lambda[q];
    }
    L_hat += albedo[q] * inner; // assume albedo[q] to be a vec3f_t
}
```

= Image Tiling
#set align(center)
#cetz.canvas(length: 10cm, {
    import cetz.draw: *

    circle((0.3, 0.15), radius: 0.01, fill: red, stroke: red)
    circle((0.3, 0.15), radius: (0.2, 0.1), stroke: red)
    circle((0.4, 0.3), radius: 0.01, fill: green, stroke: green)
    circle((0.4, 0.3), radius: (0.1, 0.15), stroke: green)
    circle((0.5, 0.2), radius: 0.01, fill: blue, stroke: blue)
    circle((0.5, 0.2), radius: (0.1, 0.1), stroke: blue)

    circle((1.3, 0.6), radius: 0.01, fill: purple, stroke: purple)
    circle((1.3, 0.6), radius: (0.275, 0.29), stroke: purple)

    rect((0,0), (rel: (1.6, 0.9)), name: "image")
    grid((0,0), (rel: (1.6, 0.9)), stroke: gray + .5pt, step: (0.4, 0.225))
})

#set align(left)
This image has a lot of empty space. We do not need to make the calculations for every gaussian for every tile of the image.
Therefore we project the centers of the gaussians onto the image plane and check if they are in a 2 sigma bounding box around the tile.

= References
projecting 3d (anisotropic) gaussians to 2d: https://www.cs.umd.edu/~zwicker/publications/EWAVolumeSplatting-VIS01.pdf

= Display Model
#set align(center)
#cetz.canvas(length: 3cm, {
    import cetz.draw: *

    content((0.5, 1.7), [CPU])
    content((2.5, 1.7), [GPU])

    set-style(mark: (symbol: ">"))
    rect((0,1), (rel: (1, 0.5)), name: "a")
    content((0.5, 1.25), [upload buffer])
    rect((2,1), (rel: (1, 0.5)), name: "b")
    rect((2,0), (rel: (1, 0.5)), name: "c")
    content((2.5, 0.25), [swapchain image])
    line("a", "b", name: "mapping")
    content(
        ("mapping.start", 0.5, "mapping.end"),
        padding: .1,
        anchor: "south",
        [mapped]
    )
    set-style(mark: (symbol: none, end: ">"))
    line("b", "c", name: "copy")
    content(
        ("copy.start", .25, "copy.end"),
        padding: .1,
        anchor: "west",
        [copy]
    )
})
