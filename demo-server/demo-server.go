/*
ESP8266 Remote Control Demo Server is a simple HTTP server for static files and
additionally provides the remote control API for testing.
*/
package main

import (
	"flag"
	"fmt"
	"math/rand"
	"net/http"
	"os"
	"strconv"
	"strings"

	"github.com/gin-contrib/static"
	"github.com/gin-gonic/gin"
	"github.com/mdzio/go-logging"
)

const (
	appDisplayName = "ESP8266 Remote Control Demo Server"
	appName        = "demo-server"
	appCopyright   = "(C)2021-2022"
	appVendor      = "info@ccu-historian.de"
	appVersion     = "1.1.0"
)

var (
	logLevel = logging.InfoLevel
	port     = flag.Int("port", 80, "port `number` of the http server")
	webapp   = flag.String("webapp", "webapp", "`directory` of the web application")
	log      = logging.Get("rc-demo-server")
)

func init() {
	flag.Var(
		&logLevel,
		"log",
		"specifies the minimum `severity` of printed log messages: off, error, warning, info, debug or trace",
	)
}

type Telemetry struct {
	// [V]
	Battery float32
	State   string
}

type Command struct {
	// -1: backward, 0: stop, 1: forward
	Throttle float32
	// -1: left, 0: straight, 1: right
	Steering float32
}

func main() {
	// parse command line
	flag.Usage = func() {
		fmt.Fprintln(os.Stderr, "usage of "+appName+":")
		flag.PrintDefaults()
	}
	// flag.Parse calls os.Exit(2) on error
	flag.Parse()
	// set log options
	logging.SetLevel(logLevel)

	// log startup message
	log.Info(appDisplayName, " V", appVersion)
	log.Info(appCopyright, " ", appVendor)

	// create web server
	gin.SetMode(gin.ReleaseMode)
	r := gin.New()
	r.SetTrustedProxies(nil)

	// add logging middleware
	r.Use(func(c *gin.Context) {
		c.Next()
		if log.TraceEnabled() {
			log.Tracef(
				"Handled request from %s, path %s, method %s; response code %d, size %d, errors: %s",
				c.ClientIP(), c.Request.URL.Path, c.Request.Method, c.Writer.Status(), c.Writer.Size(),
				strings.Join(c.Errors.Errors(), ", "),
			)
		}
	})

	// add recovery middleware
	r.Use(gin.CustomRecoveryWithWriter(nil, func(c *gin.Context, recovered interface{}) {
		log.Errorf(
			"Panic while handling request from %s, path %s, method %s: %v",
			c.ClientIP(), c.Request.URL.Path, c.Request.Method, recovered,
		)
		c.AbortWithStatus(http.StatusInternalServerError)
	}))

	// serve static files
	log.Info("Serving static files from directory ", *webapp)
	r.GET("/", func(c *gin.Context) { c.File(*webapp + "/index.htm") })
	r.NoRoute(static.Serve("/", static.LocalFile(*webapp, false)))

	// command handler
	r.PUT("/command", func(c *gin.Context) {
		var cmd Command
		if err := c.BindJSON(&cmd); err != nil {
			return
		}
		log.Debugf("Received command from %s: %+v", c.ClientIP(), cmd)
	})

	// status handler
	r.GET("/telemetry", func(c *gin.Context) {
		stat := Telemetry{
			Battery: 4.0 + rand.Float32(),
			State:   "RDY",
		}
		log.Debugf("Sending status to %s: %+v", c.ClientIP(), stat)
		c.JSON(http.StatusOK, stat)
	})

	// start server
	log.Info("Web server is listening on port ", *port)
	log.Error("Web server failed: %v", r.Run(":"+strconv.Itoa(*port)))
}
