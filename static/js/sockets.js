var socket = io();
var loginModal = new bootstrap.Modal(document.getElementById('incorrectLoginModal'), {
  keyboard: false
})


//CONNECTION AND ASK FOR LOGING
socket.once('connect', function() {
    //socket.emit('my event', {data: 'I\'m connected!'});
    console.log("CONNECTED TO SERVER");
});

//GET IF LOGIN IS VALID IN LOGIN PAGE
if(page === "LOGIN"){
  console.log("WAITING FOR FINGERPRINT INPUT");
  socket.emit('login', {data: 'I\'m connected!'});
  socket.on('loginvalid', function(data) {
      //socket.emit('my event', {data: 'I\'m connected!'});
      if(data['isValid']){
        console.log("LOGGED");
        console.log(data['fingerprint_id']);
        document.getElementById("fingerprint-input").value = data['fingerprint_id']
        document.getElementById("fingerprint-method").submit()
      }
      else {
        console.log("COULDN'T LOG");
        console.log("ASKING FOR FINGERPRINT INPUT AGAIN");
        document.getElementById("fingerprint-msg").innerHTML = data['message']
        if(!data['step']){
          socket.emit('login', {data: 'I\'m connected!'});
        }
      }
  });
}
else if (page === "SIGNUP") {
  // SEND NEW ENTRY REQUEST TO ARDUINO
  console.log("ASKING FOR NEW ENTRY");
  socket.emit('enroll', {data: 'I\'m connected!'});
  // WAIT FOR RESPONSE
  socket.on('enroll-response', function(data) {
      //socket.emit('my event', {data: 'I\'m connected!'});
      // If is valid, set id value to field and change the message i
      if(data['isValid']){
        console.log("LOG CONFIRMED");
        document.getElementById("fingerprint-input").value = data['fingerprint_id']
        document.getElementById("fingerprint-btn").className = "btn btn-success"
        document.getElementById("fingerprint-msg").innerHTML = "¡Huella capturada!"
        document.getElementById("submit-btn").disabled = false;
      }
      else {
        if(data['step']){
          document.getElementById("fingerprint-msg").innerHTML = data['message']
        }
        else {
          socket.emit('enroll', {data: 'I\'m connected!'});
        }
      }
  });
}
