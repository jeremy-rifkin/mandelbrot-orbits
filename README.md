## Mandelbrot Orbits

This is a small project looking at periodic cycles / orbits in the mandelbrot set. It's an
experimental/exploratory sort of project.

I thought this was going to be a quick one-weekend project and I hoped to solve the problem with
k-means. It turns out the problem is much more complex.

Here's the final product, an image where every bulb of the mandelbrot is colored differently based
off of the periodic orbits of points in the bulb:

![](photos/png/2k.png)

### Table of contents
* [Why This Problem Is Tricky](#why-this-problem-is-tricky)
* [The Solution](#the-solution)
* [Rendering Optimizations](#rendering-optimizations)
* [A Note About Images](#a-note-about-images)
* [Bug Journal](#bug-journal)
## Why This Problem Is Tricky

I initially wanted to solve this with k-means: run a bunch of iterations to converge on an orbit
then use k-means / the silhouette method to determine how many clusters of points there are. K-means
produced some visually cool results but definitely not solving the problem as intended.

![](photos/png/kmeans/test_20.png)

I also tried a threshold method: track previous iterations and repeatedly search for when a point
was last encountered (within some threshold). Again, cool result, but not working quite right:

![](photos/png/threshold/test.e3.fixed.png)

The problem here for both methods is that the trajectory of orbits, even simple period one orbits
in the main cardioid, can appear to spiral as they converge (see figure below). This is problematic
for k-means' cluster analysis as well as the threshold method: It can result in detecting a more
optimal clustering than desired for the period analysis and it can lead to the first point within
the threshold being a couple iterations behind even when the period is much lower (e.g. first point
within threshold suggests period = 5 when really period = 1).

Lowing the threshold to absurdly low levels (~10^-9) or trying to dynamically determine the
threshold, while making a difference in the image, does not do anything to address the main cardioid
(or other bulbs) not being colored properly.

There's something cool about the threshold method: The produced image does have have some value in
describing the "shape" of orbit trajectories. Darker color = more points/arms in the star/spiral
drawn by plotting the trajectory. Not what I was trying to produce but still a cool result.

![](photos/etc/orbit.png)
Graphic from [this](https://www.geogebra.org/m/Npd3kBKn) tool.

With both of these methods there is a lot of room for stuff to fall apart. They're both a little
hacky and susceptible to noise and flawed heuristics.

## The Solution

Nearly everything I've learned about the dynamics here is from a [lecture by Professor Benedetto](https://rlbenedetto.people.amherst.edu/talks/mhc_ug14.pdf)
in 2014. Huge thanks to him for making his slides available online.

TL;DR: There's actually a straightforward algorithm for figuring out the period of a point <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" /> in
the mandelbrot set by looking at the derivative of the nth composition of <img alt="\phi_c(z) = z^2 + c" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%28z%29%20%3D%20z%5E2%20%2B%20c" style="transform: translateY(20%);" />.

Let <img alt="\phi(z) : \mathbb{C}\to\mathbb{C}" src="https://render.githubusercontent.com/render/math?math=%5Cphi%28z%29%20%3A%20%5Cmathbb%7BC%7D%5Cto%5Cmathbb%7BC%7D" style="transform: translateY(20%);" /> be <img alt="\phi_c(z) = z^2+c" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%28z%29%20%3D%20z%5E2%2Bc" style="transform: translateY(20%);" /> and let <img alt="\phi^n_c(z)" src="https://render.githubusercontent.com/render/math?math=%5Cphi%5En_c%28z%29" style="transform: translateY(20%);" />
represent the <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" />'th iteration of the function with constant <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" />.

I.e.:

<p align="center"><img alt="\begin{align*}
\phi^1(z) &= \phi(z)\\
\phi^2(z) &= \phi \circ \phi(z)\\
\phi^3(z) &= \phi \circ \phi \circ \phi(z)\\
&...\end{align*}" src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign%2a%7D%0A%5Cphi%5E1%28z%29%20%26%3D%20%5Cphi%28z%29%5C%5C%0A%5Cphi%5E2%28z%29%20%26%3D%20%5Cphi%20%5Ccirc%20%5Cphi%28z%29%5C%5C%0A%5Cphi%5E3%28z%29%20%26%3D%20%5Cphi%20%5Ccirc%20%5Cphi%20%5Ccirc%20%5Cphi%28z%29%5C%5C%0A%26...%5Cend%7Balign%2a%7D"/></p>

All points <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" /> in the "main cardioid" (the big bulb) of the mandelbrot have period one orbits:
<img alt="\phi^n" src="https://render.githubusercontent.com/render/math?math=%5Cphi%5En" style="transform: translateY(20%);" /> converges to a single point as <img alt="n \to \infty" src="https://render.githubusercontent.com/render/math?math=n%20%5Cto%20%5Cinfty" style="transform: translateY(20%);" />.

<p align="center"><img src="/photos/etc/main_cardioid_from_lecture_slides.png"/></p>

A period one orbit (or a fixed point) is defined as <img alt="\phi_c(z)=z" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%28z%29%3Dz" style="transform: translateY(20%);" />. By applying the quadratic formula
to <img alt="z^2-z+c=0" src="https://render.githubusercontent.com/render/math?math=z%5E2-z%2Bc%3D0" style="transform: translateY(20%);" />, we can find the solutions for this equality:

<p align="center"><img alt="z = \frac{1\pm\sqrt{1-4c}}{2}" src="https://render.githubusercontent.com/render/math?math=z%20%3D%20%5Cfrac%7B1%5Cpm%5Csqrt%7B1-4c%7D%7D%7B2%7D"/></p>

Unlike real functions where one's instinct may be to say "so if this function has non-extraneous
solutions then <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" /> is in the main cardioid," every point in the complex plane will have solutions
here regardless of whether it's in the mandelbrot set or not. The question is whether <img alt="z_n" src="https://render.githubusercontent.com/render/math?math=z_n" style="transform: translateY(20%);" /> can ever
reach one of these fixed points starting with <img alt="z_0=0, z_1=c, ..." src="https://render.githubusercontent.com/render/math?math=z_0%3D0%2C%20z_1%3Dc%2C%20..." style="transform: translateY(20%);" />. We can determine whether a point
will reach one of these solutions by looking at whether or not they are attractive fixed points.

Let the multiplier <img alt="\lambda" src="https://render.githubusercontent.com/render/math?math=%5Clambda" style="transform: translateY(20%);" /> be defined as <img alt="\lambda = \phi'_c(z) = 2z" src="https://render.githubusercontent.com/render/math?math=%5Clambda%20%3D%20%5Cphi%27_c%28z%29%20%3D%202z" style="transform: translateY(20%);" />. If <img alt="\left|\lambda\right|<1" src="https://render.githubusercontent.com/render/math?math=%5Cleft%7C%5Clambda%5Cright%7C%3C1" style="transform: translateY(20%);" />,
<img alt="z" src="https://render.githubusercontent.com/render/math?math=z" style="transform: translateY(20%);" /> is an attractive fixed-point. Using this information, you can solve for all points in the main
cardioid with <img alt="\left|1\pm\sqrt{1-4c}\right|<1" src="https://render.githubusercontent.com/render/math?math=%5Cleft%7C1%5Cpm%5Csqrt%7B1-4c%7D%5Cright%7C%3C1" style="transform: translateY(20%);" />.

More generally, a point in an n-periodic orbit is expressed as <img alt="\phi_c^n(z_0)=z_0" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%5En%28z_0%29%3Dz_0" style="transform: translateY(20%);" />. The smallest
integer <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" /> such that this is true is the exact period. For a point <img alt="z" src="https://render.githubusercontent.com/render/math?math=z" style="transform: translateY(20%);" /> with exact period <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" />, the
multiplier <img alt="\lambda" src="https://render.githubusercontent.com/render/math?math=%5Clambda" style="transform: translateY(20%);" /> is <img alt="\lambda=(\phi^n)'(z)" src="https://render.githubusercontent.com/render/math?math=%5Clambda%3D%28%5Cphi%5En%29%27%28z%29" style="transform: translateY(20%);" />.

For other bulbs in the mandelbrot set with different periods, we can theoretically apply the same
solution: look for roots and test whether they are attractive. The problem is that solving for roots
of the equation <img alt="\phi^n(z_0)=z_0" src="https://render.githubusercontent.com/render/math?math=%5Cphi%5En%28z_0%29%3Dz_0" style="transform: translateY(20%);" /> quickly becomes complicated:

<p align="center"><img alt="\begin{align*}\phi(z) &= z^2 + c\\
\phi^2(z) &= (z^2 + c)^2 + c = z^4 + 2cz^2 + c^2 + c\\
\phi^n(z) &= z^{2^n}+\text{"big mess", to quote professor Benedetto}\end{align*}" src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign%2a%7D%5Cphi%28z%29%20%26%3D%20z%5E2%20%2B%20c%5C%5C%0A%5Cphi%5E2%28z%29%20%26%3D%20%28z%5E2%20%2B%20c%29%5E2%20%2B%20c%20%3D%20z%5E4%20%2B%202cz%5E2%20%2B%20c%5E2%20%2B%20c%5C%5C%0A%5Cphi%5En%28z%29%20%26%3D%20z%5E%7B2%5En%7D%2B%5Ctext%7B%22big%20mess%22%2C%20to%20quote%20professor%20Benedetto%7D%5Cend%7Balign%2a%7D"/></p>

Developing sufficiently advanced advanced numerical methods and algebraic manipulation ability to
find roots for these polynomials would be difficult. Even given those methods, solving for <img alt="2^n" src="https://render.githubusercontent.com/render/math?math=2%5En" style="transform: translateY(20%);" />
roots would be challenging and time consuming.

So here's the solution: Take advantage of the fact that if a point is in the mandelbrot set it will
converge on a fixed point / orbit (aka a root of <img alt="\phi_c^n(z_0)=z_0" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%5En%28z_0%29%3Dz_0" style="transform: translateY(20%);" />). By iterating from there we
will find other roots (approximately) of the polynomial.

If after iterating <img alt="z_{n+1} = z_n^2 + c" src="https://render.githubusercontent.com/render/math?math=z_%7Bn%2B1%7D%20%3D%20z_n%5E2%20%2B%20c" style="transform: translateY(20%);" /> many times we haven't escaped then we've approximated a
root for <img alt="\phi_c^n" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%5En" style="transform: translateY(20%);" />. In order to find <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" /> we need to check all points in potential orbits of periods
<img alt="n \in \{1, 2, ..., n_{max}\}" src="https://render.githubusercontent.com/render/math?math=n%20%5Cin%20%5C%7B1%2C%202%2C%20...%2C%20n_%7Bmax%7D%5C%7D" style="transform: translateY(20%);" /> to see if they are attractive points.

## Rendering Optimizations

Peak-performance is not the goal of this project, but, I have been able to get very good performance
by making good use of parallelization / dynamic algorithms.

```
23m ----- Plain-old single-threaded, supersampling every pixel
 2m 42.0s Parallel, supersampling every single pixel
    15.1s Single-threaded mariani-silver, adaptive anti-aliasing
     9.9s Parallel Mariani-silver parallel, adaptive anti-aliasing
```

Parallelizing the straightforward approach of throwing compute power at every single pixel reduces
time substantially, as expected. This benchmark is from a laptop with 6 cores and 12 threads. The
reason the `2m 42s` number isn't closer to `23m / 12 threads` is due to how the problem behaves
vis-a-vis hyperthread scheduling.

The next massive performance boost comes from using a smarter algorithm for rendering the mandelbrot
and adaptively determining where anti-aliasing is needed in the image.

I've implemented the Mariani-Silver algorithm here, taking advantage of the fact that the mandelbrot
is connected. We can compute all points on the edge of a box and if they're all the same then the
entire square can be filled without spending compute time on the inside. If the edge is not uniform,
the box is subdivided. This algorithm speeds things up drastically.

![](photos/png/mariani.png)

In this figure, orange pixels are the only points where computational time was expended in
discovering the mandelbrot's main body. The entire outside could be excluded but I am taking escape
time into account here so we don't miss any detail around the edge of the set. It's still super
fast.

Mariani-silver lends itself well to a depth-first recursive strategy and it's very fast with, just a
couple seconds even on a high resolution render. But, I've parallelized it using a multi-producer
multi-consumer thread pool. This parallelization makes the already-fast computation nearly instant.

After the main structure of the mandelbrot is found an edge detection pass is done to figure out
where anti-aliasing is needed. Points near the perimeter of the mandelbrot are queued and a thread
pool tackles the problem set. When new detail is discovered by the anti-aliasing workers they'll
queue more points to investigate. The points which are supersampled are highlighted in red below:

![](photos/png/adaptiveaa.png)

Much better than super-sampling the whole image.

With the final render time brought down by a factor of 140x, anti-aliasing is still the slowest by
far (mariani-silver is nearly instant). There's some more room for optimization with the
anti-aliasing (both with how work is queued and the fairly basic super-sampling technique). GPU
anti-aliasing could also possibly be explored later.

## A Note About Images

The program spits out uncompressed bmp files because that's easy but these can be fairly large. To
compress them (losslessly) use imagemagick's `convert` tool.

## Bug Journal

Here lie a bunch of visually cool results which resulted from failed attempts at solving this
problem.

![](photos/png/kmeans/test_20_m4.png)

```
k-means with some parameters I've forgotten xD
more kmeans bugs in photos/(bmp|png)/kmeans
```

![](photos/png/threshold/test.hmm.png)

```
Initial commit
40 iterations
10 seed points
float threshold = min_distance / 2;
max 40 iterations
```

![](photos/png/threshold/test_bw_more_iters.png)

```
Low-iteration low-threshold
```

![](photos/png/threshold/test.edge.png)

```
Second commit
40 iterations
10 seed points
any points which escape in the following 40 iterations get colored red
```

![](photos/png/threshold/test.edgenodiv2.png)

```
Higher-fidelity threshold, third commit
```

![](photos/png/test.lambda.png)

```
Just coloring the main-body with a hard-coded equation
```

![](photos/png/test.first_success.png)

```
This was the first semi-successful render
```

![](photos/png/test.cooooool_result.1kiters.3col.png)

```
A bit more detected but period-detection still buggy
```

![](photos/png/test-aa.png)

```
Iirc here there was an issue with considering points which escaped at 40 iterations as part of the
lambda calculation. The gray anomalies were detected as period 10 (value = 20 * period).
```

![](photos/png/test.interesting.png)

```
I forget what caused this
```

![](photos/png/test.interesting2.png)

```
I forget what caused this
```

![](photos/png/test.anotherinterestingone.png)

```
For some reason detail was detected but some bulbs didn't detect properly, I forget the reason.
Notice how there are some small bulbs which are two or three colored. Interesting stuff.
```

![](photos/png/test.interesting3.png)

```
Low iteration render. Iterations have to be pretty high for high-resolution renders with this
algorithm in order for points near the edge to converge sufficiently.
```
