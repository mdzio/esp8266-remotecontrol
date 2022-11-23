/*
Program generate-css-layout generates the file layout.css.
*/
package main

import (
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"math"
	"strings"
)

const maxSubdivisions = 5

var filePath = flag.String("o", "layout.css", "`path` of the output file")

func main() {
	log.SetFlags(0)
	flag.Parse()
	var builder bytes.Buffer
	allClasses := make([]string, 0)
	for _, orientation := range []string{"h", "v"} {
		for subdivisions := 2.0; subdivisions <= maxSubdivisions; subdivisions++ {
			for startDivision := 1.0; startDivision <= subdivisions; startDivision++ {
				for endDivision := startDivision; endDivision <= subdivisions; endDivision++ {
					name := fmt.Sprintf(".%v-%v-%v-%v", orientation, subdivisions, startDivision, endDivision)
					start := percent(1 / subdivisions * (startDivision - 1))
					length := percent(1 / subdivisions * (endDivision - startDivision + 1))
					builder.WriteString(name + "{")
					if orientation == "h" {
						builder.WriteString(fmt.Sprintf("left:%v%%;width:%v%%;height:100%%", start, length))
					} else {
						builder.WriteString(fmt.Sprintf("top:%v%%;width:100%%;height:%v%%", start, length))
					}
					builder.WriteRune('}')
					allClasses = append(allClasses, name)
				}
			}
		}
	}
	builder.WriteString(strings.Join(allClasses, ","))
	builder.WriteString("{margin:0;padding:0;border:0;position:absolute}")
	err := ioutil.WriteFile(*filePath, builder.Bytes(), 0644)
	if err != nil {
		log.Fatalln(err)
	}
	log.Println("File", *filePath, "written.")
}

func percent(f float64) float64 {
	return math.Floor(f*10000) / 100
}
