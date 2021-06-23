## Mandelbrot Orbits

This is a small project looking at periodic cycles / orbits in the mandelbrot set. It's an
experimental/exploratory sort of project.

I thought this was going to be a quick one weekend project and I hoped to solve the problem with
kmeans. It turns out the problem was much more complex.

Here's the final product, an image where every bulb of the mandelbrot is colored differently based
off of the periodic orbits of points in the bulb: TODO

### Table of contents
* [The Math](#the-math)
* [Rendering optimizations](#rendering-optimizations)
* [Bug Journal](#bug-journal)
## The Math

Nearly everything I've learned about the dynamics here is from a [lecture by Professor Benedetto](https://rlbenedetto.people.amherst.edu/talks/mhc_ug14.pdf)
in 2014. Huge thanks to him for making his slides available online.

TL;DR: There's actually a very straightforward algorithm for figuring out the period of a point <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" />
in the mandelbrot set by looking at the derivative of the nth composition of <img alt="\phi_c(z) = z^2 + c" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%28z%29%20%3D%20z%5E2%20%2B%20c" style="transform: translateY(20%);" />.

Let <img alt="\phi(z) : \mathbb{C}\to\mathbb{C}" src="https://render.githubusercontent.com/render/math?math=%5Cphi%28z%29%20%3A%20%5Cmathbb%7BC%7D%5Cto%5Cmathbb%7BC%7D" style="transform: translateY(20%);" /> be <img alt="\phi_c(z) = z^2+c" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%28z%29%20%3D%20z%5E2%2Bc" style="transform: translateY(20%);" /> and let <img alt="\phi^n_c(z)" src="https://render.githubusercontent.com/render/math?math=%5Cphi%5En_c%28z%29" style="transform: translateY(20%);" />
represent the <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" />'th iteration of the function with constant <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" />.

I.e.:

<p align="center"><img alt="\begin{align*}
\phi^1(z) &= \phi(z)\\
\phi^2(z) &= \phi \circ \phi(z)\\
\phi^3(z) &= \phi \circ \phi \circ \phi(z)\\
&...\end{align*}" src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign%2a%7D%0A%5Cphi%5E1%28z%29%20%26%3D%20%5Cphi%28z%29%5C%5C%0A%5Cphi%5E2%28z%29%20%26%3D%20%5Cphi%20%5Ccirc%20%5Cphi%28z%29%5C%5C%0A%5Cphi%5E3%28z%29%20%26%3D%20%5Cphi%20%5Ccirc%20%5Cphi%20%5Ccirc%20%5Cphi%28z%29%5C%5C%0A%26...%5Cend%7Balign%2a%7D"/></p>

All points <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" /> in the "main cardiod" (the big bulb) of the mandelbrot have period one orbits:
<img alt="\phi^n" src="https://render.githubusercontent.com/render/math?math=%5Cphi%5En" style="transform: translateY(20%);" /> converges to a single point as <img alt="n \to \infty" src="https://render.githubusercontent.com/render/math?math=n%20%5Cto%20%5Cinfty" style="transform: translateY(20%);" />.

![](photos/_main_cardiod_from_lecture_slides.png)

A period one orbit (or a fixed point) is defined as <img alt="\phi_c(z)=z" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%28z%29%3Dz" style="transform: translateY(20%);" />. By applying the quadratic formula
to <img alt="z^2-z+c=0" src="https://render.githubusercontent.com/render/math?math=z%5E2-z%2Bc%3D0" style="transform: translateY(20%);" />, we can find the solutions for this equality:

<p align="center"><img alt="z = \frac{1\pm\sqrt{1-4c}}{2}" src="https://render.githubusercontent.com/render/math?math=z%20%3D%20%5Cfrac%7B1%5Cpm%5Csqrt%7B1-4c%7D%7D%7B2%7D"/></p>

Unlike real functions where one's instinct may be to say "so if this function has real
non-extraneous solutions then <img alt="c" src="https://render.githubusercontent.com/render/math?math=c" style="transform: translateY(20%);" /> is in the main cardioid," every point in the complex plane will
have solutions here regardless of whether it's in the mandelbrot set or not. The question is whether
<img alt="z_n" src="https://render.githubusercontent.com/render/math?math=z_n" style="transform: translateY(20%);" /> can ever reach one of these fixed points starting with <img alt="z_0=0, z_1=c, ..." src="https://render.githubusercontent.com/render/math?math=z_0%3D0%2C%20z_1%3Dc%2C%20..." style="transform: translateY(20%);" />. We can determine
whether a point will reach one of these solutions by looking at whether or not they are attractive
fixed points.

Let the multiplier <img alt="\lambda" src="https://render.githubusercontent.com/render/math?math=%5Clambda" style="transform: translateY(20%);" /> be defined as <img alt="\lambda = \phi'_c(z) = 2z" src="https://render.githubusercontent.com/render/math?math=%5Clambda%20%3D%20%5Cphi%27_c%28z%29%20%3D%202z" style="transform: translateY(20%);" />. If <img alt="\left|\lambda\right|<1" src="https://render.githubusercontent.com/render/math?math=%5Cleft%7C%5Clambda%5Cright%7C%3C1" style="transform: translateY(20%);" />,
<img alt="z" src="https://render.githubusercontent.com/render/math?math=z" style="transform: translateY(20%);" /> is an attractive fixed-point. Using this information, you can solve for all points in the main
cardiod with <img alt="\left|1\pm\sqrt{1-4c}\right|<1" src="https://render.githubusercontent.com/render/math?math=%5Cleft%7C1%5Cpm%5Csqrt%7B1-4c%7D%5Cright%7C%3C1" style="transform: translateY(20%);" />.

More generally, a point in an n-periodic orbit is expressed as <img alt="\phi_c^n(z_0)=z_0" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%5En%28z_0%29%3Dz_0" style="transform: translateY(20%);" />. The smallest
integer <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" /> such that this is true is the exact period. For a point <img alt="z" src="https://render.githubusercontent.com/render/math?math=z" style="transform: translateY(20%);" /> with exact period <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" />, the
multiplier <img alt="\lambda" src="https://render.githubusercontent.com/render/math?math=%5Clambda" style="transform: translateY(20%);" /> is <img alt="\lambda=(\phi^n)'(z)" src="https://render.githubusercontent.com/render/math?math=%5Clambda%3D%28%5Cphi%5En%29%27%28z%29" style="transform: translateY(20%);" />

For other bulbs in the mandelbrot set with different periods, we can theoretically apply the same
solution: look for roots and test whether they are attractive. The problem is that solving for roots
of the equation <img alt="\phi^n(z_0)=z_0" src="https://render.githubusercontent.com/render/math?math=%5Cphi%5En%28z_0%29%3Dz_0" style="transform: translateY(20%);" /> quickly becomes complicated:

<p align="center"><img alt="\begin{align*}\phi(z) &= z^2 + c\\
\phi(z)^2 &= (z^2 + c)^2 + c = z^4 + 2cz^2 + c^2 + c\\
\phi(z)^n &= z^{2^n}+\text{"big mess", to quote professor Benedetto}\end{align*}" src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign%2a%7D%5Cphi%28z%29%20%26%3D%20z%5E2%20%2B%20c%5C%5C%0A%5Cphi%28z%29%5E2%20%26%3D%20%28z%5E2%20%2B%20c%29%5E2%20%2B%20c%20%3D%20z%5E4%20%2B%202cz%5E2%20%2B%20c%5E2%20%2B%20c%5C%5C%0A%5Cphi%28z%29%5En%20%26%3D%20z%5E%7B2%5En%7D%2B%5Ctext%7B%22big%20mess%22%2C%20to%20quote%20professor%20Benedetto%7D%5Cend%7Balign%2a%7D"/></p>

Developing sufficiently advanced advanced numerical methods and algebraic manipulation ability to
find roots for these polynomials would be difficult. Even given those methods, solving for <img alt="2^n" src="https://render.githubusercontent.com/render/math?math=2%5En" style="transform: translateY(20%);" />
roots would be challenging and time consuming.

So here's the solution: Take advantage of the fact that if a point is in the mandelbrot set it will
converge on a fixed point / orbit (aka a root of <img alt="\phi_c^n(z_0)=z_0" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%5En%28z_0%29%3Dz_0" style="transform: translateY(20%);" />). If after iterating <img alt="\phi" src="https://render.githubusercontent.com/render/math?math=%5Cphi" style="transform: translateY(20%);" /> a
bunch we haven't escaped then we've approximated a root for <img alt="\phi_c^n" src="https://render.githubusercontent.com/render/math?math=%5Cphi_c%5En" style="transform: translateY(20%);" /> for some value <img alt="n" src="https://render.githubusercontent.com/render/math?math=n" style="transform: translateY(20%);" />. We can
then check all points in orbit of various periods <img alt="n \in {1, 2, ..., n_{max}}" src="https://render.githubusercontent.com/render/math?math=n%20%5Cin%20%7B1%2C%202%2C%20...%2C%20n_%7Bmax%7D%7D" style="transform: translateY(20%);" /> to see if they are
attractive points.

## Rendering optimizations

## Bug Journal

Here lie a bunch of visually cool results which resulted from failed attempts at solving this
problem.
