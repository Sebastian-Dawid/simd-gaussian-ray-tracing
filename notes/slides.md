---
title: Accelerating Volumetric Ray Tracing using SIMD Instructions
author: Sebastian Dawid (sdawid@techfak.uni-bielefeld.de)
---

Agenda
===

* Algorithm
    * Top Level View
    * Maths
    * Implementation
* Parallelization
    * How can the algorithm be parallelized?
    * What are the challanges?
* Preliminary Results

<!--end_slide-->

Algorithm
===

# Ray Tracing

```typst +render
#import "@preview/cetz:0.2.2"
#set align(center)
#cetz.canvas(length: 10cm, {
    import cetz.draw: *

    circle((1.0, 0.5), radius: 0.01, fill: red, stroke: red)
    
    intersections("i", {
        line((0, 0), (1.5, 0.6), close: true, stroke: white)
        circle((1.0, 0.5), radius: 0.25, stroke: red)
    })
    for-each-anchor("i", (name) => {
        circle("i." + name, radius: .01, fill: white)
    })
})
```

<!--end_slide-->
