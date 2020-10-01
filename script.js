const mobileDebug = (data) => {
   const Http = new XMLHttpRequest();
   const url = `http://192.168.1.45:3000/?d=${data}`;
   Http.open("GET", url);
   Http.send();
}

let color = {
   h: 0,
   s: 100,
   l: 50
};

let state = false;

const setNewPos = (event) => {
   event = event || window.event;
   window.e = event;
   const width = (document.getElementById('color').width.animVal.value) / 2;
   let xCoord, yCoord;
   if (event.type.includes("touch")) {
      //this is fucking cursed but it sometimes works 
      event.preventDefault();
      const MAGIC_NUMBER = 2.9;
      xCoord = event.layerX - width / MAGIC_NUMBER - width;
      yCoord = (-1 * event.layerY) + width / MAGIC_NUMBER + width;
   } else {
      xCoord = event.offsetX - width;
      yCoord = (-1 * event.offsetY) + width;
   }
   const colorWheelSize = document.getElementById('colorwheel').r.baseVal.value;
   let thetaDeg = Math.atan(yCoord / xCoord) * 180 / Math.PI;
   if (xCoord < 0) { //Q2 + Q3
      thetaDeg += 180;
   } else {
      if (yCoord < 0) { //Q4
         thetaDeg += 360;
      }
   }
   color.h = thetaDeg;

   document.getElementById("powerbutton").style.stroke = `hsl(${color.h}, 100%, 50%)`;
   document.getElementById("saturation").style.setProperty("--bg-color", `hsl(${color.h}, 100%, 50%)`);
   document.getElementById("dragbutton").setAttribute("cx", ((975 * Math.cos((thetaDeg / 180) * Math.PI)) + colorWheelSize));
   document.getElementById("dragbutton").setAttribute("cy", ((975 * Math.sin((thetaDeg / 180) * Math.PI)) * -1 + colorWheelSize));

   sendData();

   document.getElementById("colorwheel").onmousemove = setNewPos;
   document.getElementById("colorwheel").onmouseup = clearDrag;
   document.getElementById("colorwheel").ontouchmove = setNewPos;
   document.getElementById("colorwheel").ontouchend = clearDrag;
}

const clearDrag = () => {
   document.getElementById("dragbutton").onmouseup = document.getElementById("colorwheel").removeEventListener("mousemove", setNewPos);
   document.getElementById("colorwheel").onmousemove = null;
   document.getElementById("colorwheel").onmouseup = null;
   document.getElementById("colorwheel").ontouchmove = null;
   document.getElementById("colorwheel").ontouchend = null;
}

const togglePower = () => {
   document.getElementById('power').classList.toggle("off");
   clearDrag();
   state = !state;
}

document.querySelectorAll('.touchwheel').forEach(item => {
   item.addEventListener("mousedown", setNewPos);
   item.addEventListener("touchstart", setNewPos);
})

Array.from(document.getElementById('power').children).forEach(item => {
   item.addEventListener('click', togglePower);
})

const setSaturation = (value) => {
   color.s = value;
   sendData();
}

const setLightness = (value) => {
   color.l = parseInt(value);
   document.getElementById('lightness').style.setProperty('--percent', value + "%");
   sendData();
}

const sendData = () => {
   if (state) {
      console.log(`sending hsl(${color.h}, ${color.s}, ${color.l}) to ESP8266`);
      let HTTP = new XMLHttpRequest();
      let url = `http://192.168.1.65:81/setColor?h=${color.h}&s=${color.s}&l=${color.l}`;
      
   }
}