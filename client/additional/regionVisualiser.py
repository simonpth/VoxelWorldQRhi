import tkinter as tk

CELL_SIZE = 20
GRID_RADIUS = 20  # grid on each side

class RegionViewer:
    def __init__(self, root):
        self.root = root
        root.title("Pixel Circle Render Distance")

        self.canvas_size = CELL_SIZE * (GRID_RADIUS * 2 + 1)
        self.canvas = tk.Canvas(root, width=self.canvas_size, height=self.canvas_size, bg="white")
        self.canvas.pack()

        self.slider = tk.Scale(root, from_=0, to=GRID_RADIUS, orient="horizontal",
                               label="regionRenderDistance", command=self.redraw)
        self.slider.set(3)
        self.slider.pack(fill="x")

        self.redraw()

    def get_circle_points(self, r):
        """Get filled discrete circle points using midpoint algorithm."""
        pts = set()
        # Midpoint circle algorithm for circumference
        x = r
        y = 0
        decision = 1 - r

        while x >= y:
            for ix in range(-x, x+1):  # fill between points
                pts.add((ix,  y))
                pts.add((ix, -y))
            for ix in range(-y, y+1):
                pts.add((ix,  x))
                pts.add((ix, -x))

            y += 1
            if decision <= 0:
                decision += 2 * y + 1
            else:
                x -= 1
                decision += 2 * (y - x) + 1
        return pts

    def redraw(self, *_):
        self.canvas.delete("all")

        r = self.slider.get()
        circle = self.get_circle_points(r)
        center = GRID_RADIUS

        for x in range(-GRID_RADIUS, GRID_RADIUS+1):
            for z in range(-GRID_RADIUS, GRID_RADIUS+1):
                cx = (x + center) * CELL_SIZE
                cz = (z + center) * CELL_SIZE

                self.canvas.create_rectangle(cx, cz, cx+CELL_SIZE, cz+CELL_SIZE, outline="#ddd")
                if (x, z) in circle:
                    self.canvas.create_rectangle(cx, cz, cx+CELL_SIZE, cz+CELL_SIZE,
                                                 fill="#4a90e2", outline="#4a90e2")

        # Origin
        ox = center * CELL_SIZE
        oz = center * CELL_SIZE
        self.canvas.create_rectangle(ox, oz, ox+CELL_SIZE, oz+CELL_SIZE, fill="red", outline="black")


if __name__ == "__main__":
    root = tk.Tk()
    RegionViewer(root)
    root.mainloop()
