## Mandelbrot Orbits

This is a small project looking at periodic cycles / orbits in the mandelbrot set. It's an
experimental/exploratory sort of project.

I thought this was going to be a quick one weekend project and I hoped to solve the problem with
kmeans. It turns out the problem was much more complex.

Here's the final product, an image where every bulb of the mandelbrot is colored differently based
off of the periodic orbits of points in the bulb: TODO

### Table of contents
- [The Math](#the-math)
- [Rendering optimizations](#rendering-optimizations)
- [Bug Journal](#bug-journal)

## The Math

Nearly everything I've learned about the dynamics here is from a [lecture by Professor Benedetto][1]
in 2014. Huge thanks to him for making his slides available online.

TL;DR: There's actually a very straightforward algorithm for figuring out the period of a point $c$
in the mandelbrot set by looking at the derivative of the nth composition of $\phi_c(z) = z^2 + c$.

Let $\phi(z) : \mathbb{C}\to\mathbb{C}$ be $\phi_c(z) = z^2+c$ and let $\phi^n_c(z)$
represent the $n$'th iteration of the function with constant $c$.

I.e.:

$$\begin{align*}
\phi^1(z) &= \phi(z)\\
\phi^2(z) &= \phi \circ \phi(z)\\
\phi^3(z) &= \phi \circ \phi \circ \phi(z)\\
&...\end{align*}$$

All points $c$ in the "main cardiod" (the big bulb) of the mandelbrot have period one orbits:
$\phi^n$ converges to a single point as $n \to \infty$.

![](photos/_main_cardiod_from_lecture_slides.png)

A period one orbit (or a fixed point) is defined as $\phi_c(z)=z$. By applying the quadratic formula
to $z^2-z+c=0$, we can find the solutions for this equality:

$$z = \frac{1\pm\sqrt{1-4c}}{2}$$

Unlike real functions where one's instinct may be to say "so if this function has real
non-extraneous solutions then $c$ is in the main cardioid," every point in the complex plane will
have solutions here regardless of whether it's in the mandelbrot set or not. The question is whether
$z_n$ can ever reach one of these fixed points starting with $z_0=0, z_1=c, ...$. We can determine
whether a point will reach one of these solutions by looking at whether or not they are attractive
fixed points.

Let the multiplier $\lambda$ be defined as $\lambda = \phi'_c(z) = 2z$. If $\left|\lambda\right|<1$,
$z$ is an attractive fixed-point. Using this information, you can solve for all points in the main
cardiod with $\left|1\pm\sqrt{1-4c}\right|<1$.

More generally, a point in an n-periodic orbit is expressed as $\phi_c^n(z_0)=z_0$. The smallest
integer $n$ such that this is true is the exact period. For a point $z$ with exact period $n$, the
multiplier $\lambda$ is $\lambda=(\phi^n)'(z)$

For other bulbs in the mandelbrot set with different periods, we can theoretically apply the same
solution: look for roots and test whether they are attractive. The problem is that solving for roots
of the equation $\phi^n(z_0)=z_0$ quickly becomes complicated:

$$\begin{align*}\phi(z) &= z^2 + c\\
\phi(z)^2 &= (z^2 + c)^2 + c = z^4 + 2cz^2 + c^2 + c\\
\phi(z)^n &= z^{2^n}+\text{"big mess", to quote professor Benedetto}\end{align*}$$

Developing sufficiently advanced advanced numerical methods and algebraic manipulation ability to
find roots for these polynomials would be difficult. Even given those methods, solving for $2^n$
roots would be challenging and time consuming.

So here's the solution: Take advantage of the fact that if a point is in the mandelbrot set it will
converge on a fixed point / orbit (aka a root of $\phi_c^n(z_0)=z_0$). If after iterating $\phi$ a
bunch we haven't escaped then we've approximated a root for $\phi_c^n$ for some value $n$. We can
then check all points in orbit of various periods $n \in {1, 2, ..., n_{max}}$ to see if they are
attractive points.

[1]: https://rlbenedetto.people.amherst.edu/talks/mhc_ug14.pdf

## Rendering optimizations

## Bug Journal

Here lie a bunch of visually cool results which resulted from failed attempts at solving this
problem.
