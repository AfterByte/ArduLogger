import serial, time
from flask import Flask, request, render_template, redirect, url_for, session
from flask_socketio import SocketIO, send, emit
import mysql.connector as mysql

# FLASK AND SOCKETS CONFIG
app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret'
socketio = SocketIO(app)
socketio.run(app)


# ARDUINO CONNECTION
try:
    arduino = serial.Serial('/dev/ttyACM0', baudrate=9600) #LINUX
except:
    arduino = serial.Serial('COM3', baudrate=9600) #WINDOWS
arduino.setDTR(False)
time.sleep(1)
arduino.flushInput()
arduino.setDTR(True)


# DATABASE CONFIG
database = mysql.connect(
    host="localhost",
    user="root",
    passwd="",          #PUT YOUR PASSWORD HERE
    database="ardulogger"
)

database_cursor = database.cursor()

#### HELPER ####
def check_user(email=None, password=None, fingerprint=None):
    if fingerprint:
        print(f"CHECKING {fingerprint}")
        sql_sentence = "SELECT name FROM users WHERE fingerprint=%s"
        database_cursor.execute(sql_sentence, (fingerprint,))
    else:
        print(f"CHECKING {email} {password}")
        sql_sentence = "SELECT name FROM users WHERE email=%s and password=%s"
        database_cursor.execute(sql_sentence, (email, password))
    results = database_cursor.fetchall()
    if len(results) == 1:
        return results[0][0]
    return False



#### WEB URLS ####

# LOGIN
@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'GET':
        return render_template('login.html')
    else:
        print("TRYING TO LOG:")
        print(request.form.get("fingerprint-input"))
        if request.form.get("fingerprint-input", False):
            print("LOGGING BY FINGERPRINT")
            fingerprint = request.form.get("fingerprint-input", None)
            user = check_user(fingerprint=fingerprint)
            if user:
                print("USER FOUND")
                session['user'] = user
                return redirect('/')
            else:
                print("USER NOT FOUND")
                return render_template('login.html', error="Datos incorrectos")

        else:
            print("NORMAL LOGGING")
            email = request.form.get("email")
            password = request.form.get("password")
            if check_user(email, password):
                print("USER FOUND")
                session['user'] = email
                return redirect('/')
            else:
                print("USER NOT FOUND")
                return render_template('login.html', error="Datos incorrectos")


# LOGOUT
@app.route('/logout')
def logout():
    session.pop('user', None)
    return redirect('/')


# ACCESS GRANTED VIEW
@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'GET':
        if 'user' in session:
            return render_template('index.html')
        else:
            return redirect('/login')


# SIGN UP VIEW
@app.route('/signup', methods=['GET', 'POST'])
def signup():
    if request.method == 'GET':
        return render_template('signup.html')
    else:
        # RETRIEVING POSTED VALUES
        name = request.form.get("name")
        email = request.form.get("email")
        password = request.form.get("password")
        fingerprint = request.form.get("fingerprint")
        # TUPLE
        values = (name, email, password, fingerprint)
        print("-"*50)
        print(name)
        print(email)
        print(password)
        print(fingerprint)
        print("-"*50)
        sql_sentence = "INSERT INTO users (name, email, password, fingerprint) VALUES (%s, %s, %s, %s)"
        try:
            database_cursor.execute(sql_sentence, values)
            database.commit()
            session['user'] = name
        except:
            error_str = "El email está duplicado"
            return render_template('signup.html', error=error_str)
        return redirect('/')




#### WEB SOCKETS ####

# LOGIN VALIDATION
@socketio.on('login', namespace="/")
def handle_message(message):
    print("-"*50)
    print("WAITING FOR LOGIN BY FINGERPRINT")
    global arduino
    #SEND MODE
    arduino.write("2".encode())
    time.sleep(1)
    #RETRIEVE MESSAGE
    # DOING ENROLL PROCESS
    line = ""
    while "ERROR: " not in line:
        try:
            line = str(arduino.readline())[2:-5]
            print(f"'{line}' READED")
            if "ID:" in line:
                line = line.replace("ID: ", "")
                print(f"VERIFICATION SUCCESSFUL '{line}'")
                emit("loginvalid",{'isValid': True, 'fingerprint_id':line})
                break
            elif "STEP: "in line:
                print("STEP RECEIVED")
                line = line.replace("STEP: ","")
                emit("loginvalid",{'isValid': False, 'step': True, 'message': line})
            else:
                print("ERROR IN PROCESS")
                emit("loginvalid",{'isValid': False, 'step': False, 'message': line})
                break
        except:
            print("ERROR: THERE IS A PROBLEM WITH ARDUINO")
    print("DONE")

# NEW FINGERPRINT ENTRY (ARDUINO)
@socketio.on('enroll')
def new_entry(message):
    print("-"*50)
    print("ENROLLING WAITING FOR ENTRY")
    global arduino
    # SETTING SENSOR TO ENROLL MODE
    arduino.write("1".encode())
    # CALCULATING ID FOR FINGERPRINT RECORD AND SENDING IT TO ARDUINO
    sql_sentence = f"SELECT id FROM users ORDER BY id DESC"
    database_cursor.execute(sql_sentence,())
    results = database_cursor.fetchall()
    if len(results) == 0:
        fp_id = 1
    else:
        fp_id = results[0][0] + 1
    time.sleep(1)
    arduino.write(str(fp_id).encode())
    print(f"SENDING {fp_id} ID")
    # DOING ENROLL PROCESS
    line = ""
    while "ERROR: " not in line:
        try:
            line = str(arduino.readline())[2:-5]
            print(f"'{line}' READED")
            if "ID:" in line:
                print("ENROLLMENT SUCCESSFUL")
                line = line.replace("ID: ","")
                emit("enroll-response",{'isValid': True, 'fingerprint_id':line})
                break
            elif "STEP: "in line:
                print("STEP RECEIVED")
                line = line.replace("STEP: ","")
                emit("enroll-response",{'isValid': False, 'step':True, 'message': line})
            else:
                print("ERROR IN PROCESS")
                emit("enroll-response",{'isValid': False, 'step':False, 'message': line})
                break
        except:
            print("ERROR: THERE IS A PROBLEM WITH ARDUINO")
    print("DONE")
