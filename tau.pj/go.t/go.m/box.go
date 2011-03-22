// Draw a box

package main

import (
	"fmt"
//	"exp/draw"
	"exp/draw/x11"
	"image"
)

func main() {
	color := image.RGBAColor{255, 255, 255, 255}
//	red := image.RGBAColor{255, 255, 0, 255}
//	var color image.Color
//	color = 0xff00ff
//	color.RGBA(1000, 0, 0, 1000)
	c, err := x11.NewWindow()
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	s := c.Screen()
/*
	for x := 0; x < 100; x++ {
		for y := 0; y < 200; y++ {
			color = image.RGBAColor{uint8(x), uint8(y), 0, 255}
			s.Set(x, y, color)
		}
	}
*/
	b := s.Bounds()
	for x := 0; x < b.Max.Y; x++ {
		y := x*x;
		if y >= b.Max.X { break; }
		s.Set(x, y, color)
	}
	c.FlushImage();
	fmt.Printf("Press any key to exit.\n")
	<-c.EventChan();
}
/*
        s := c.Screen(); 
        for x := 0; x < min(s.Width(), m.Width()); x++ { 
                for y := 0; y < min(s.Height(), m.Height()); y++ { 
                        s.Set(x, y, m.At(x, y)); 
                } 
        } 
        c.FlushImage(); 
        fmt.Printf("Press the any key to exit.\n"); 
        for { 
                select { 
                case <- c.KeyboardChan(): 
                        return; 
                case <- c.MouseChan(): 
                        // No-op. 
                case <- c.ResizeChan(): 
                        // No-op. 
                case <- c.QuitChan(): 
                        return; 
                } 
        }
        
*/
