/*
 * Random Alogirthm for Iterated Function System.
 * Fractals Everywhere by Barnsley page 91
 */

package main

import (
	"exp/draw"
	"exp/draw/x11"
	"flag"
	"fmt"
	"image"
	"math"
	"rand"
	"strconv"
)

type Ifs struct {
	a, b, c, d, e, f, p	float64
}

type Point struct {
	x, y float64
}

var (
	waitForGraphics = flag.Bool("w", false, "wait for graphics")
	inlineGraphics  = flag.Bool("i", false, "inline the call to plot")

	Sierpinski = []Ifs{
		Ifs{ 0.5, 0, 0, 0.5,  1,  1, 0.33 },
		Ifs{ 0.5, 0, 0, 0.5,  1, 50, 0.33 },
		Ifs{ 0.5, 0, 0, 0.5, 50, 50, 0.34 }}

	Square = []Ifs{
		Ifs{ 0.5, 0, 0, 0.5,  1,  1, 0.25 },
		Ifs{ 0.5, 0, 0, 0.5, 50,  1, 0.25 },
		Ifs{ 0.5, 0, 0, 0.5,  1, 50, 0.25 },
		Ifs{ 0.5, 0, 0, 0.5, 50, 50, 0.25 }}

	Fern = []Ifs{
		Ifs{  0,     0,     0,    0.16, 0, 0,    0.01 },
		Ifs{  0.85,  0.04, -0.04, 0.85, 0, 1.6,  0.85 },
		Ifs{  0.2,  -0.26,  0.23, 0.22, 0, 1.6,  0.07 },
		Ifs{ -0.15,  0.28,  0.26, 0.24, 0, 0.44, 0.07 }}

	Tree = []Ifs{
		Ifs{ 0,     0,     0,    0.16, 0, 0,   0.01 },
		Ifs{ 0.42, -0.42,  0.42, 0.42, 0, 0.2, 0.40 },
		Ifs{ 0.42,  0.42, -0.42, 0.42, 0, 0.2, 0.40 },
		Ifs{ 0.1,   0,     0,    0.1,  0, 0.2, 0.15 }}

	Color = image.RGBAColor{0, 0xff, 0, 0xff}
	black = image.RGBAColor{0, 0, 0, 0xff}

	Scale float64 = 3
	Shrink float64 = 0.90
)

func scale(v []Point) (min Point, max Point) {
	min = Point{ math.MaxFloat64,  math.MaxFloat64}
	max = Point{-math.MaxFloat64, -math.MaxFloat64}
	for _, p := range v {
		if p.x > max.x { max.x = p.x }
		if p.x < min.x { min.x = p.x }
		if p.y > max.y { max.y = p.y }
		if p.y < min.y { min.y = p.y }
	}
	return min, max
}

func clear(s draw.Image) {
	b := s.Bounds()
	for i := b.Min.X; i < b.Max.X; i++ {
		for j := b.Min.Y; j < b.Max.Y; j++ {
			s.Set(i, j, black);
		}
	}
}

func plot(s draw.Image, v []Point) {
	clear(s)
	min, max := scale(v)
	b := s.Bounds()
	xscale  := float64(b.Max.X - b.Min.X)/(max.x - min.x) * Shrink
	yscale  := float64(b.Max.Y - b.Min.Y)/(max.y - min.y) * Shrink
	xoffset := -(min.x * xscale) + 4
	yoffset := -(min.y * yscale) + 4
	for _, p := range v {
		x := int(xscale * p.x + xoffset)
		y := int(yscale * p.y + yoffset)
if x < 0 || y < 0 {
	fmt.Println(min, max)
	fmt.Println(x, y)
	fmt.Println(p.x, xscale, xoffset)
	fmt.Println(p.y, yscale, yoffset)
}
		s.Set(x, y, Color)
	}
}

func goplot(w draw.Window, ch chan []Point, wait chan int) {
	s := w.Screen()

	for {
		v := <-ch
		plot(s, v)
		w.FlushImage();
		if *waitForGraphics {
			wait <- 1
		}
	}
}

func inlineplot(w draw.Window, v []Point) {
	s := w.Screen()

	plot(s, v)
	w.FlushImage();
}

const Skip = 100;

func prob(ifs []Ifs) (p [100]*Ifs) {
	k := 0
	for i := range ifs {
		n := int(float64(len(p)) * ifs[i].p)
		for j := 0; j < n; j++ {
			p[k] = &ifs[i]
			if k++; k >= len(p) { return p }
		}
	}
	for i := k; i < len(p); i++ {
		p[i] = p[rand.Int() % k];
	}
	return p
}

func ifs(v []Point, ifs []Ifs) {
	var f *Ifs
	var x, y float64

	p := prob(ifs)

	x = 0; y = 0
	for i := 0; i < Skip; i++ {
		f = p[rand.Int() % len(p)]
		x, y = x*f.a + y*f.b + f.e, x*f.c + y*f.d + f.f
	}
	for i := range v {
		f = p[rand.Int() % 100]
		x, y = x*f.a + y*f.b + f.e, x*f.c + y*f.d + f.f
		v[i] = Point{x, y}
	}
}

func main() {
	flag.Parse()
	n := 100000
	if flag.NArg() > 0 {
		n, _ = strconv.Atoi(flag.Arg(0))
	}
	if flag.NArg() > 1 {
		Scale, _ = strconv.Atof64(flag.Arg(1))
	}
	w, err := x11.NewWindow()
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	v := make([]Point, n)
	ch := make(chan []Point)
	wait := make(chan int)

	if !*inlineGraphics {
		go goplot(w, ch, wait)
	}

	pic := Fern
	s := w.Screen()
	b := s.Bounds()
	fmt.Printf("Press 'q' to exit.\n\tinline: %v\n\twait:   %v.\n",
		*inlineGraphics, *waitForGraphics)
	ifs(v, pic)
	if *inlineGraphics {
		inlineplot(w, v)
	} else {
		ch <- v
		if *waitForGraphics {
			<-wait
		}
	}
	for event := range w.EventChan() {
		switch e := event.(type) {
		case draw.KeyEvent:
			if (e.Key > 0) {
				switch e.Key {
				case 'c': clear(w.Screen());
				case 'f': pic = Fern
				case 'k': pic = Sierpinski
				case 's': pic = Square
				case 't': pic = Tree
				case 'q': return
				default: fmt.Println("key=", e.Key)
				}
			}
		case draw.MouseEvent:
			if (e.Buttons != 0) {
				clear(s)
			}
			red := uint8((e.Loc.X << 8) / b.Max.X)
			green := uint8((e.Loc.Y << 8) / b.Max.Y)
			Color = image.RGBAColor{red, green, 0, 0xff}
			//fmt.Println("mouse=", m)
		case draw.ConfigEvent:
			s = w.Screen()
			b = s.Bounds()
			fmt.Println("Bounds: ", b)
			return
		}
		ifs(v, pic)
		if *inlineGraphics {
			inlineplot(w, v)
		} else {
			ch <- v
			if *waitForGraphics {
				<-wait
			}
		}
	}
	/*
	for {
		ifs(v, pic)
		if *inlineGraphics {
			inlineplot(c, v)
		} else {
			ch <- v
			if *waitForGraphics {
				<- wait
			}
		}
		select {
		case key := <- c.KeyboardChan():
			if (key > 0) {
				switch key {
				case 'c': clear(c.Screen());
				case 'f': pic = Fern
				case 'k': pic = Sierpinski
				case 's': pic = Square
				case 't': pic = Tree
				case 'q': return
				default: fmt.Println("key=", key)
				}
			}
		case m := <- c.MouseChan():
			if (m.Buttons != 0) {
				clear(s)
			}
			red := uint8((m.Point.X << 8) / b.Max.X)
			green := uint8((m.Point.Y << 8) / b.Max.Y)
			Color = image.RGBAColor{red, green, 0, 0xff}
			//fmt.Println("mouse=", m)
		case resize := <- c.ResizeChan():
			fmt.Println("Resize=", resize)
		case <- c.QuitChan():
			return
		}
	}
	*/
}
