
package main

import (
	"exp/draw"
	"exp/draw/x11"
	"fmt"
	"image"
)

type Point struct {
	X, Y float64
}

type Range struct {
	Min, Max float64
}

type Graph struct {
	window	draw.Window
	domain, yrange	Range
	color image.Color
	xscale, yscale float64
	height int
}

func Round(x float64) int {
	if x >= 0.0 {
		return int(x + 0.5)
	}
	return int(x - 0.5)
}

func (g *Graph) Plot(p Point) {
	x := p.X - g.domain.Min
	y := p.Y - g.yrange.Min
	w := Round(x * g.xscale)
	h := g.height - Round(y * g.yscale)
	g.window.Screen().Set(w, h, g.color)
	g.window.FlushImage();
}

func (g *Graph) setScale() {
	b := g.window.Screen().Bounds()
	dw := (float64)(b.Max.X - b.Min.X)
	dh := (float64)(b.Max.Y - b.Min.Y)
	dx := g.domain.Max - g.domain.Min
	dy := g.yrange.Max - g.yrange.Min
	g.xscale = dw / dx
	g.yscale = dh / dy
	g.height = b.Max.Y
}

func NewGraph(domain, yrange Range) (g Graph) {
	w, err := x11.NewWindow()
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	g.window = w
	g.domain = domain
	g.yrange = yrange
	g.color  = image.RGBAColor{0, 0xff, 0, 0xff}
	g.setScale();
	return
}

func logisticMap(g Graph) {
	for k := 2.5; k < 4.0; k += 0.001 {
		x := 0.5
		for i := 0; i < 200; i++ {
			x = k * x * (1.0 - x)
		}
		for i := 0; i < 100; i++ {
			x = k * x * (1.0 - x)
			g.Plot(Point{k, x})
			
		}
	}
}

func quadratic(g Graph) {
	for x := 0.0; x < 1000.0; x += 0.001 {
		y := x * x
		g.Plot(Point{x, y})
	}
}

func cmd(g Graph) {
	i := 0
	for event := range g.window.EventChan() {
		switch e := event.(type) {
		case draw.KeyEvent:
			if (e.Key > 0) {
				switch e.Key {
				case 'q': return
				default: fmt.Println("key=", e.Key)
				}
			}
		default:
			i++
			fmt.Println("event type: ", event, i);
		}
	}
}

func main() {
	g := NewGraph(Range{2.5, 4.0}, Range{0.0, 1.0})
	logisticMap(g)
	h := NewGraph(Range{0, 100}, Range{0, 10000})
	quadratic(h)
	cmd(g)
}
