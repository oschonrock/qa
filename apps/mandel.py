import numpy as np
import matplotlib.pyplot as plt


def mandelbrot(height, width, x=-0.5, y=0, zoom=1, max_iterations=100):
    # To make navigation easier we calculate these values
    x_width = 1.5
    y_height = 1.5*height/width
    x_from = x - x_width/zoom
    x_to = x + x_width/zoom
    y_from = y - y_height/zoom
    y_to = y + y_height/zoom

    # Here the actual algorithm starts
    x = np.linspace(x_from, x_to, width).reshape((1, width))
    y = np.linspace(y_from, y_to, height).reshape((height, 1))
    c = x + 1j * y

    # Initialize z to all zero
    z = np.zeros(c.shape, dtype=np.complex128)
    # To keep track in which iteration the point diverged
    div_time = np.zeros(z.shape, dtype=int)
    # To keep track on which points did not converge so far
    m = np.full(c.shape, True, dtype=bool)

    for i in range(max_iterations):
        z[m] = z[m]**2 + c[m]

        diverged = np.greater(np.abs(z), 2, out=np.full(c.shape, False), where=m) # Find diverging

        div_time[diverged] = i      # set the value of the diverged iteration number
        m[np.abs(z) > 2] = False    # to remember which have diverged
    return div_time


# Default image of Mandelbrot set
plt.imshow(mandelbrot(800, 1000), cmap='magma')
# The image below of Mandelbrot set
# plt.imshow(mandelbrot(800, 1000, -0.75, 0.0, 2, 200), cmap='magma')
# The image below of below of Mandelbrot set
# plt.imshow(mandelbrot(800, 1000, -1, 0.3, 20, 500), cmap='magma')
plt.show()
