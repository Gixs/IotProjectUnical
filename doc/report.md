
# Smart Lab Sensor <!-- omit in toc -->
## Low level and Embedded System programming Project Report <!-- omit in toc -->

## ***Luigi Rachiele*** - Computer Engineering of IoT - ***214894*** <!-- omit in toc -->

***

## Table of Contents <!-- omit in toc -->

- [Intro](#intro)
- [Project Scope](#project-scope)
- [Architecture](#architecture)
- [Southbound](#southbound)
  - [Southbound architecture](#southbound-architecture)
  - [Sensing](#sensing)
  - [Actuation](#actuation)
    - [States](#states)
    - [User to Actuation Interaction](#user-to-actuation-interaction)
  - [ATmega328p - Arduino uno board](#atmega328p---arduino-uno-board)
  - [FreeRTOS](#freertos)
    - [**Sensors** and **ADC**](#sensors-and-adc)
    - [UART](#uart)
  - [ESP8266](#esp8266)
    - [main.cpp](#maincpp)
- [Cloud](#cloud)
    - [SSH Key](#ssh-key)
    - [Ubuntu UFW](#ubuntu-ufw)
  - [MQTT Broker - Mosquitto](#mqtt-broker---mosquitto)
    - [Let's Encrypt certificate](#lets-encrypt-certificate)
    - [Mosquitto configuration](#mosquitto-configuration)
  - [Database](#database)
  - [Node-RED](#node-red)
  - [Data flow](#data-flow)
  - [Node-RED - *Encrypt and Store*](#node-red---encrypt-and-store)
  - [Node-RED - *Query and Decrypt*](#node-red---query-and-decrypt)
- [Northbound](#northbound)
  - [Android App](#android-app)
    - [MqttHandler.dart](#mqtthandlerdart)
    - [Front End](#front-end)
  - [NodeRED Dashboard](#nodered-dashboard)
    - [Back-end](#back-end)
    - [Front-end](#front-end-1)
- [Conclusion](#conclusion)

## Intro

The following report provides an overview and analysis of the **Low Level and Embedded** System programming aspects implemented in a project focused on developing a **smart lab sensor**. The primary objective of this project is to create an advanced embedded system capable of detecting the presence of toxic gases and fire incidents in laboratory. The embedded system is designed to enable prompt actions in response to safety alerts, such as activating a fan for air circulation and sounding a buzzer to alert users.

The project emphasizes the utilization of low-level programming techniques and embedded systems to enhance safety and security measures. By leveraging advanced sensing capabilities, the system can accurately identify the presence of toxic gases and fire incidents, providing timely warnings to mitigate potential risks.

The embedded system's architecture and programming techniques are designed to ensure efficient data acquisition, processing, and transmission. Through the use of low-level programming, precise control over the hardware components is achieved, optimizing system resources and enhancing overall performance and reliability.

Throughout this report, the implementation details of the embedded system are explored, emphasizing the utilization of low-level programming techniques tailored to the specific requirements of the project. The report also highlights the significance of integrating various sensors, such as temperature sensors, to enhance the system's functionality and improve safety measures.

Additionally, the report examines the implementation of actuation mechanisms, including fan activation for air circulation and buzzer activation for alerting users during safety alerts. The utilization of low-level programming ensures efficient control and seamless integration of these actuation mechanisms within the embedded system.

By analyzing the role of low-level programming and embedded systems in the project, this report aims to provide a comprehensive understanding of their importance in developing smart smoke sensors capable of detecting toxic gases and fire incidents. The insights and recommendations derived from this analysis contribute to the broader field of Low Level and Embedded System programming, offering valuable guidance for future research and development in safety-enhancing embedded systems.

## Project Scope

The project has been designed to address the safety needs of a chemical laboratory where chemical gas releases and fires may occur. Its main purpose is to ensure a safe working environment for the laboratory personnel, prevent potential incidents, and minimize the negative effects resulting from such situations.

The key features of the project include timely detection of chemical gas releases and fires, immediate alerting of the personnel, and implementation of mitigation measures to minimize the harmful effects. The specific objectives of the project are outlined below:

1. Detection of chemical gas releases: The implemented system allows for the real-time identification and monitoring of any leaks or releases of chemical gases within the laboratory. Using specially designed sensors, the system can detect the presence of hazardous substances in the air and generate an immediate alert to the laboratory personnel.

2. Fire detection: The project also encompasses early fire detection. Smoke, heat, and flame sensors are strategically placed in the laboratory to promptly detect signals of potential fires. These sensors trigger an alarm and send an immediate notification to the personnel so they can take necessary evacuation and extinguishing actions.

3. Alerting and notifications: When a chemical gas release or fire is detected, the system activates an audible and visual alarm within the laboratory to immediately alert the personnel present. Simultaneously, automatic notifications are sent to the safety officers and designated personnel through mobile devices or an internal communication system.

4. Mitigation measures: In addition to timely alerts, the project includes the implementation of mitigation measures to minimize the negative effects of chemical gas releases and fires. These measures may involve activating ventilation systems, automatically closing gas valves, isolating specific areas, and deploying specialized teams to handle the emergency.

By implementing this project, the chemical laboratory will significantly enhance the safety of its operations. The timely detection of chemical gas releases and fires, coupled with appropriate alerting and mitigation measures, will help protect the personnel, prevent material damages, and preserve the surrounding environment.

The project aims to provide the laboratory with a solid safety foundation, enabling the personnel to carry out their activities in a controlled and protected environment, in accordance with the best industrial safety practices.

## Architecture

The architecture comprises three essential components, as mentioned befor: the **Southbound**, **Cloud**, and **Northbound**.

The Southbound component encompasses the physical device responsible for sensing environmental data, such as temperature and air quality, as well as managing alarms triggered by threshold exceedances. It also includes actuating mechanisms, including a buzzer for alerting users to potential dangers and a fan for enhancing air circulation.

The Cloud component focuses on the cloud infrastructure, incorporating an MQTT broker (Mosquitto) for reliable device communication, a database (MySQL) for secure data storage, and Node-RED for efficient data flow management.

Lastly, the Northbound component consists of user-facing applications, namely the Android App and Node-RED Dashboard, providing intuitive interfaces for data visualization and interaction. By integrating these three components, the architecture ensures a secure, efficient, and user-friendly IoT system capable of safeguarding data privacy, detecting anomalies, and facilitating informed decision-making.

<p align="center">
  <img src="images/system_architecture.png" alt="System architecture">
</p>

The overall system architecture is illustrated in the  image. This image provides a visual representation of how the different components interact and highlights the protocol used within the system.

## Southbound

This section provides an overview of the part of the project where data are generated.
The generation of the data that flow in the architecture are sensed from the external and are in the form of Temperature (°) and Quality of Air.
The data are sensed using the physical device discussed below.

### Southbound architecture
The Southbound architecture is a crucial component of the embedded system and it is based on the utilization of the ATmega328P microcontroller. It consist of four main parts. It leverages various technologies and modules to enable seamless operation and interaction within the system.

**FreeRTOS**: At the core of the Southbound architecture lies FreeRTOS, a real-time operating system specifically designed for microcontrollers like the ATmega328P. FreeRTOS provides advanced task management, scheduling, and resource handling capabilities, enabling efficient multitasking and optimal utilization of system resources.
**Sensing**: The sensing part forms an integral component of the Southbound architecture, enabling data acquisition from the environment. It incorporates two sensors, namely TMP36 and MQ135. The TMP36 sensor measures temperature, while the MQ135 sensor detects the presence of toxic gases. These sensors interface directly with the ATmega328P, allowing the system to monitor and respond to environmental changes effectively.

**Actuation**: The actuation part of the Southbound architecture empowers the embedded system to execute appropriate actions based on the collected sensor data. It consists of two actuators, a fan and a buzzer, which are controlled by the ATmega328P. In response to detected hazardous conditions, the system can activate the fan to circulate air and employ the buzzer to provide audible alarms, ensuring prompt and necessary interventions.

**User Interaction**: User interaction is facilitated through an LCD (Liquid Crystal Display) and the integration of an ESP8266 device. The LCD provides a visual interface for displaying relevant information, while the ESP8266 allows the Arduino board to exchange data with the cloud. This capability opens up possibilities for remote monitoring, data logging, and control of the embedded system.

The Southbound architecture brings together these interconnected components, enabling a comprehensive and efficient embedded system. It leverages FreeRTOS for task management, utilizes sensors for data acquisition, employs actuators for response mechanisms, and incorporates user interaction elements for enhanced control and monitoring. Together, these components form a cohesive architecture that ensures reliable and intelligent operation of the embedded system.

<p align="center">
  <img src="images/southbound_architecture.png" alt="Southbound architecture">
</p>

### Sensing

### Actuation

The implementation of actuation in your project involves two separate components: a fan for air circulation and a buzzer for alarm signaling. Both of these components operate by utilizing Pulse Width Modulation (PWM) techniques.

The **buzzer**'s PWM modulation is achieved through the utilization of vTaskDelay in FreeRTOS. This allows for precise control over the buzzer's output. On the other hand, the fan's PWM modulation is achieved using a hardware timer of the ATmega328P microcontroller.

The **fan** operates based on the principles of DC motors, and to enable its functionality, additional support components are employed. Firstly, a small DC motor typically requires more power than what the UNO R3 board can directly provide. Attempting to connect the motor directly to a pin of the UNO board would pose a high risk of damaging the board. To address this, a separate power supply is used to ensure the motor receives the appropriate power.

In addition to the power supply, a chip like the *L293D* is utilized to control the motor's operation. The L293D provides the necessary interface between the microcontroller and the motor, allowing for precise control and protection against potential electrical issues.

Lastly, the inclusion of a *optoisolator*, called *4N35*. An optoisolator, also known as an optocoupler, is a device that electrically isolates two circuits while allowing them to communicate through light signals.  The optoisolator consists of an LED (Light-Emitting Diode) on the input side and a phototransistor or a photothyristor on the output side. When a voltage is applied to the input side, the LED emits light, which is detected by the phototransistor/photothyristor on the output side. This optical coupling allows for electrical isolation between the input and output circuits. Power a DC motoro whitout and optoisolator is very dangerous, becaus if the motor changes the polarity, in an inusual way, a negative flow of current goes to the AVR and can cause problem to the chip.

By combining the PWM modulation techniques, appropriate power supply management, motor control chip, and optoisolator, the actuation system in your project ensures efficient and reliable operation of both the fan and the buzzer, serving their respective purposes in the overall functionality of the system.

#### States
The state the system can ensure are different. The table list every Actuation state.


| #   |      Name      |  FreeRTOS Task | What does? |
|---|:----------:|-----------:|------:|
| 0 | Alarm ON   | alarmON    |Start alarm. -> Start Buzzer and Fan|
| 1 | Alarm OFF  | alarmOFF   |Stop alarm. -> Stop Buzzer and Fan|
| 2 | Buzzer ON  | buzzerTask |Start Buzzer|
| 3 | Buzzer OFF | buzzerTask |Stop Buzzer|
| 4 | Fan ON     | fanTask    |Start Fan at Speed 1|
| 5 | Fan OFF    | fanTask    |Stop Fan|
| 6 | Fan x2     | fanTask    |Start Fan at Speed 2x|






#### User to Actuation Interaction
Through the dashboard interface, users have the ability to issue various commands to the microprocessor, enabling them to perform specific actions. These commands include turning the alarm on and off, activating or deactivating the buzzer, and controlling the fan by turning it on or off.

Each of these commands directly affects the state of the system and is managed through task scheduling using the FreeRTOS operating system. The system comprises different tasks, namely "fanTask," "alarmOn," "alarmOff," and "buzzerTask," which are responsible for executing the corresponding actions based on the received commands.

To facilitate communication between the user and the microprocessor, the MQTT protocol is employed. When a command is sent by the user through the northbound dashboard, it is transmitted via MQTT. The ESP8266, which acts as the MQTT listener, receives the command and forwards it to the Arduino board using the serial port for further processing.

Upon receiving the command through the serial communication, a system interrupt is triggered. This interrupt suspends the execution of other tasks momentarily, prioritizing the handling of the received command. Consequently, the appropriate task is awakened to process the command and carry out the necessary actions.

It's worth noting that the commands can vary in type, depending on the desired state of the system. Each distinct command serves as a trigger to awaken a specific task within the FreeRTOS framework, ensuring efficient and accurate actuation based on the user's input.

### ATmega328p - Arduino uno board

The ATmega328P is an 8-bit, 28-pin, low-power microcontroller manufactured by Atmel. It is the most popular microcontroller used in Arduino boards. The ATmega328P has a maximum clock speed of 20 MHz and 32 KB of flash memory, 2 KB of SRAM, and 1 KB of EEPROM. It also has a wide range of interfaces, including a serial port, a parallel port, a timer, and an analog-to-digital converter.

The ATmega328P is based on the development board Arduino Uno. It is one of the most popular Arduino boards. It is easy to use and has a large community of users and developers who can provide support and assistance.

The **Arduino Uno** acts as a pivotal board housing the ATMega328P microprocessor, providing the necessary computational power for the project. Within the Arduino Uno, several components are employed to facilitate the system's functionality.

In the project are used the ISR (interrupt service routine), PORTB and PORTD and the hardware timer.
### FreeRTOS
FreeRTOS plays a crucial role in the embedded system project described, as it manages the Southbound section of the system. Its task management, timing, and inter-process communication features are essential to this project's seamless functioning.

In our project, the smoke sensor's core functions and applications hinge on the efficient use of FreeRTOS. Here, FreeRTOS is tasked with managing the operations of the TMP36 and MQ135 sensors, a fan, a buzzer, an LCD for user interaction, and the ESP8266 device for cloud communication. Each component runs as a separate task in the FreeRTOS environment, enabling simultaneous operation and synchronization.

The TMP36 sensor is used for temperature detection, and the MQ135 sensor is used to detect the presence of harmful gases. When the sensor data exceeds a safety threshold, FreeRTOS ensures the immediate triggering of specific responses - activating the fan for air circulation and turning on the buzzer to alert the user.

//TODO TASK THRESHOLD DOPO IL SENSE

The FreeRTOS scheduler manages these tasks, coordinating the task-switching based on priority and ensuring that the system responds to hazards promptly. It also helps manage the communication with the ESP8266 device, providing a crucial link between the Southbound hardware components and the cloud.

When a message comes from the Northbound to the MQTT Broker the ESP32 send through serial the message received in

The usage of FreeRTOS in this project allows the intelligent smoke sensor to operate efficiently and reliably, responding to real-time changes in the environment while also communicating critical data to the cloud, ensuring safety and promoting smart home automation. Furthermore, it exemplifies the effective use of low-level programming in managing and coordinating multiple hardware components in a single, compact system.
#### **Sensors** and **ADC**

For the sensing phase, two distinct analogic sensors are utilized: the MQ135 and the TMP36.

<p align="center">
  <img src="images/tmp36.jpg" alt="TMP36 Temperature sensor." width="300">
  <img src="images/mq135.jpg" alt="MQ135 Air Quality Sensor." width="200">
</p>

The MQ135 sensor is employed to monitor the quality of the surrounding air, enabling the system to detect any potential air pollution or harmful gases. On the other hand, the TMP36 sensor is responsible for measuring the ambient temperature, providing valuable data for temperature monitoring purposes. These sensors are directly connected to the Arduino Uno, leveraging its analog-to-digital converter (ADC) capabilities to convert the analog sensor readings into digital values that can be processed by the microprocessor.

By leveraging the Arduino Uno's versatile capabilities and integrating the MQ135 and TMP36 sensors, the system can accurately sense and monitor both the air quality and temperature. This information serves as crucial input for subsequent stages of the project, such as generating alerts in case of threshold exceedance or initiating the actuating phase for appropriate actions. The Arduino Uno acts as a reliable and robust foundation, enabling seamless integration between the physical sensing components and the overall IoT system architecture.

The chosen approach involves manipulating the internal registers of the ATmega328P microprocessor to ensure a robust, secure, optimized, and direct code implementation. This method provides precise control over the microprocessor's functionalities, allowing for customization according to the project's specific requirements.

``` C
void adc_setup_temp(void){
  DDRC&=~(1<<0); //DDRC pin 0 as input for ADC
  ADCSRA = 0x87; //make ADC enagle and select ck/128
  ADMUX = 0b11000000; //Vref Internal

}
```

``` C
void adc_setup_air(void){
  DDRC&=~(1<<1); //DDRC pin 1 as input for ADC
  ADCSRA = 0x87; //make ADC enagle and select ck/128
  ADMUX = 0b01000001; //Vref AVCC pin

}
```

In this code snippet, has been used direct the register manipulation to configure the ADC registers for analog-to-digital conversion. By setting the appropriate bits in the ADMUX and ADCSRA registers, they select the ADC input and enable it with a specific prescaler value.

After the configuration, an ADC conversion is initiated by setting the ADSC bit, and the code waits for the conversion to complete. Once the conversion is finished, the raw ADC value is retrieved converted into voltage using the known voltage reference.

This approach of manipulating the microprocessor's internal registers provides precise control over its functionalities, resulting in an optimized and tailored code implementation for the project.

#### UART

In order to enable message transmission from the sensors to the cloud, the project utilizes the MQTT protocol. However, a crucial requirement for this implementation is establishing an internet connection. This is achieved by utilizing an ESP8266 module, which comes equipped with built-in Wi-Fi connectivity, to connect to an accessible hotspot. To facilitate data transfer from the Arduino to the ESP8266, the project leverages the UART (Universal Asynchronous Receiver-Transmitter) serial communication. By connecting the RX (receive) and TX (transmit) pins of both the Arduino and the ESP8266, a seamless data flow between the two devices is established.

First of all the UART is initialized:

``` C
void UART_init()
{
    // Setup Trasmission rate
    UBRR0H = (BAUD_PRESCALER >> 8);
    UBRR0L = BAUD_PRESCALER;

    // Enable both in and out trasmission
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // Setup data format: 8 data bit, 1 stop
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

```

Then 2 different function permits the data trasmission:

``` C

void UART_sendString(const char* data)
{
    // Send each character until the null terminator
    while (*data != '\0')
    {
        UART_sendChar(*data);
        data++;
    }
}

```

``` C
void UART_sendChar(char data)
{
    // Wait until the trasmission buffer is empty
    while (!(UCSR0A & (1 << UDRE0)));

    // Upload the data in the trasmission buffer
    UDR0 = data;
}

```

```UART_sendChar``` wait until the trasmission buffer is empty and then upload the data in the USART Data Register 0. When the data are written on the UDR, it will be trasnferred by the UART peripheral through the serial transimt pin.

### ESP8266

The ESP8266 receives from the UART the data from the Arduino UNO and sends it using the Trasport Layer Security (TLS). The connection to the cloud is established using the MQTT (Message Queuing Telemetry Transport) protocol enriched with the TLS (Transport Layer Security) security protocol. This combination ensures a secure and protected connection between the ESP8266 device and the cloud. By using the generated certificates and stored in the device flash memory, the ESP8266 device can establish a secure connection with the cloud. These certificates allow for the verification of the device and server identities in the TLS handshake process, thereby preventing potential man-in-the-middle attacks.

The ESP8266 code is composed by 3 parts:
- *ConnectionManager*: this module manage the certs file acquiring the MQTT connection using TLS.
- *Service*: the goal of this part is to actually create the MQTT Client;
- *main*: this is the main part where the flow of data goes through.

#### main.cpp

This module describe the *main* part of the sketch.
It takes all the *secrets* information, that are loaded in the Flash Memory of the ESP8266 and build the MQTT client.

Once the connection is enstablished, the ESP8266 has one and only task: forward the serial message received by the Arduino board to the MQTT Broker.
To do that, the ESP8266 start the serial monitor, scan for messages and when it receives one forward that pubblishing a message in the 'D001' topic.

## Cloud

The cloud section focuses on the cloud infrastructure and services employed in the IoT Security project. It includes various components critical to the IoT Security project, including the MQTT broker, database, and the utilization of Node-RED for data flow management.

The cloud is managed using a Digital Ocean droplet that serves as the central point of the project.

#### SSH Key

The communication for the configuration of the server is accomplished by using SSH key. Another form of security thanks to disabling of the password login of the SSH.

#### Ubuntu UFW

In the ubuntu droplet is also used the *Uncomplicated Firewall* aka UFW. To ensure that unwanted connection are unallowed.

### MQTT Broker - Mosquitto

The MQTT broker plays a pivotal role in facilitating communication between the IoT devices and the cloud services. It acts as a central hub, receiving messages from the devices and distributing them to the relevant cloud services. The MQTT broker's implementation includes robust security measures to ensure the confidentiality, integrity, and availability of the transmitted data. This involves the use of secure authentication protocols, access controls, and encryption techniques to protect against unauthorized access and data breaches.

The chosen MQTT broker for the project is Mosquitto. The communication between the different subscribers is secured using TLS (Transport Layer Security). Additionally, username and password authentication is implemented to ensure authorized access to the MQTT broker.

To manage the TLS protocol, Certbot is utilized, and the project obtains certificates through "Let's Encrypt". The obtained certificates are associated with the domain "iotprojectunical.me", which is acquired from the namecheap website.

By leveraging TLS, username and password authentication, and obtaining certificates from Let's Encrypt, the project establishes a secure and encrypted communication channel between the MQTT broker and the subscribers. This setup ensures the confidentiality, integrity, and authenticity of the transmitted data within the IoT Security project.

Three foundamental user are created for authentication of Mosquitto Broker:

- *mqttesp*: used by the esp to enstablish an MQTT connection with the broker;
- *mqttnodered*: used by the Node-RED flow;
- *mqttandroid*: used by the Android App;

#### Let's Encrypt certificate

To acquire the SSL certificates to enable the TLS MQTT connection is used a nonprofit Cerficate Authority called *Let's Encrypt*.
For accomplish the certificate is used a tool called *Certbot*.

The SSL cerficate need a real existing domain and the one used in this project is acquired from the *Namecheap* web hosting agency.

#### Mosquitto configuration

To enable the Mosquitto TLS communication it is needed to create new configuration file in the mosquitto *conf.d*.

The file contain the following information:

``` Bash
allow_anonymous false
password_file /etc/mosquitto/passwd
listener 1883 localhost
listener 8883
certfile /etc/letsencrypt/live/iotprojectunical.me/cert.pem
cafile /etc/letsencrypt/live/iotprojectunical.me/chain.pem
keyfile /etc/letsencrypt/live/iotprojectunical.me/privkey.pem
```

First of all disallow the anonymous publish and subscribe.
Then enable the password file where the password are saved in a sha256 encryption.
Setup then the cerficate obtained by the Certbot on the namecheap domain: *iotprojectunical.me*

### Database

In the database section, we discuss the database management system utilized in the IoT Security project.

The database utilized in the project is MySQL. However, for the encryption of the database, a deliberate decision was made not to employ Transparent Data Encryption (TDE). Instead, a manual approach to data encryption was deemed more suitable for educational purposes. Consequently, the data is manually encrypted using the DES (Data Encryption Standard) algorithm before being inserted into the database.

By employing manual encryption, the project gains greater control over the encryption process and can apply specific encryption techniques tailored to its requirements. This approach allows for the secure storage of sensitive data within the database.

When retrieving the data from the database, it can be decrypted, enabling various functions and operations to be performed on the decrypted data. This provides the necessary flexibility for data analysis, manipulation, and processing while maintaining the security of the sensitive information.

The decision to opt for manual data encryption with DES rather then of TDE demonstrates a purposeful choice made within the project for educational purposes. It offers insights into the manual encryption process and provides an opportunity to gain a deeper understanding of data encryption techniques and their implementation within a secure environment.

The table where the data are stored are created by using the following code:

``` SQL
CREATE TABLE readings (
  device varchar(4) NOT NULL,
  datetime TIMESTAMP NOT NULL,
  sensor varchar (20) NOT NULL,
  value varchar(44) NOT NULL,
  PRIMARY KEY (device, datetime, sensor)
) ENGINE = InnoDB

```

So a *redings* table is created to store both temperature and air quality readings.
It has a different column describing the various information of the stored data:

- **device** value to show from wich device the reading comes; in this project is used just one device;
- **Timestamp** value;
- **sensor** value that store if it is a Temperature or Air Quality reading;
- **value** part that store the encrypted form of the Float calculated by the Arduino sensors.

The Primary Key is setup by a triple of device, datetime and sensor.

### Node-RED

Regarding Node-RED, it is also hosted in the cloud. It is protected by username and password authentication, and access to it is secured using the HTTPS protocol through the implementation of the Nginx web server.

Node-RED plays a crucial role in managing the flow of data within the project. Data is received by Node-RED through MQTT, ensuring secure communication with TLS encryption. From there, the data is directed towards two main destinations. Firstly, it flows towards the dashboard, which provides a visual representation of the data for monitoring and analysis. Secondly, the data is routed towards the encryption phase, where it is encrypted before being securely stored in the database.

By employing username and password authentication and implementing HTTPS through Nginx, Node-RED ensures secure access and communication. The integration with MQTT, along with TLS encryption, guarantees the confidentiality and integrity of the data as it traverses through the system. The combined functionalities of Node-RED, MQTT with TLS, and the secure storage of data in the database contribute to the overall security of the IoT Security project.

The Node-RED app is protected by an authentication system.

<p align="center">
  <img src="images/node-red_auth.png" alt="node-RED Auth" width="600">
</p>

First of all is inject in the system the AES key created in the Cloud using the *OpenSSL Tool*.

<p align="center">
  <img src="images/node-RED_inject_key.png" alt="node-RED Inject Key">
</p>

Then the Node-RED MQTT node subscribe to all the topic of the Mosquitto Broker, using the wildcard "#".
Data that flow to the Mosquitto broker flow into 2 different subflow, one for the Dashboard and the other one for the ***Encription and Store*** phase.

The third phase of the Node-RED flow start from the user input on the Dashboard and follow 2 flow. One to setup some command on the Arduino Board and one for the ***Query and Decript*** phase.

### Data flow

Before goin in the deep for the Node-RED different phase let's talk about the system **Data Flow**.
The data flow from Arduino to the Database are in JSON format.
This is an example of *readings*.

``` JSON
{"sensor":"temperature","value": 26.33}"
```

The JSON is composed by a map of keyvalue of size 2. The first key/value indicates the sensor used for the reading (e.g. Temperature, Air);
the second key/value couple indicates the value of the reading.

Those data has to flow in Node-RED, has to be encrypted and then stored in the MySQL database.

> The encription could have been done in two different way in this system: **TDE Method** and **Manual method**.
The TDE method (Transparent data encryption) executes encryption and decryption within the database engine itself. This method doesn’t require code modification of the database or application and is easier for admins to manage. However is not the best eligible in a didactic enviroment. The manual method is the best fit for understanding how encryption works and to study also the AES encryption method.

For this project, it is chosen to encrypt only the value part of the JSON data. The encryption is made by using the AES algorithm given by a Node-RED node.

> #### **AES**
>
> AES (Advanced Encryption Standard) is a widely used encryption algorithm that ensures the security and confidentiality of data. It operates on blocks of data and employs a symmetric key, meaning the same key is used for both encryption and decryption.
>
> In AES encryption, the data is divided into fixed-size blocks and processed through multiple rounds of transformation, including substitution, permutation, and mixing operations. These operations, performed on both the data and the encryption key, make AES highly secure and resistant to cryptographic attacks.
>
>AES supports three key sizes: 128 bits, 192 bits, and 256 bits. The larger the key size, the stronger the encryption. The chosen key size determines the number of rounds applied during the encryption process.
>
>To encrypt data using AES, the plaintext is first divided into blocks and padded if necessary. Then, the encryption key is applied to each block, resulting in a ciphertext. Decryption involves applying the same key to the ciphertext, reversing the encryption process, and obtaining the original plaintext.
>
>AES encryption provides robust protection for sensitive information, making it suitable for a wide range of applications, including secure communication, data storage, and cryptographic protocols. Its widespread adoption and recognition as a standard encryption algorithm highlight its effectiveness in ensuring data confidentiality and integrity.

The data pass through the Node-RED flow and is redirected both into the Dashboard as clear data and in the Encrytpion node.
After the Encryption the data is stored in the MySQL database.

From the Dashboard, an user can ask to the Database some datatime based query (see in the Dashboard paragraph). The query is applied to the database and the encrypted data is given as output. The data is then decrypted and is shown in the Dashboard.

### Node-RED - *Encrypt and Store*

The first Node-RED subflow analyzed is the *Encrypt and Store*.

![Node-RED Encrypt and Store module](images/Node-RED_encrypt_and_store.png)

The input messages are in the JSON form. The infos that the system need from the message in input are:
- *Device*: which device is sending information; it can be retrieved from the MQTT topic;
- *Sensor*: which device sensor is reading; it can be retrieved from the JSON in input;
- *Value*: which value has the reading; it can be retrieved from the JSON in input;

Every message that flow in this subflow has to pass in the Encryption node.
The Encryption node take the *msg.key* and the *msg.payload* and makes the AES encryption.

After the encryption the message has to be purged from the key (or it will be shown in clear) and to be sent in the *function node* that build the query for the Database.
An example of query is shown belove:

``` SQl

USE sensors;
INSERT INTO Readings(device, datetime, sensor, value) VALUES('D001', NOW(), 'air', '120');`

```

The query has to be set in the *msg.topic* and has to flow in the *MySQL* node.

### Node-RED - *Query and Decrypt*

The second Node-RED sublflow analyzed is the *Query and Decrypt*.

![Node-RED Query and Decrypt module](images/Node-RED_query_and_decrypt.png)

The input this time is from the user dashboard, analyzed below.

After the user input the data are converted in good format to be joined in a Query for the database.
This an example query for the database:

``` SQl

USE sensors;
SELECT * FROM Readings WHERE device='D001'
  AND sensor='temperature'
  AND datetime>='2023-06-26'
  AND datetime<'2023-06-27';

```

After the DB interrogation the output is decrypted and from them are generated two different output: a **table** with all value founded from the query and a **graph** with the curves that the sensing made in the datatime range.

## Northbound

The Northbound section of this report focuses on the applications that transparently utilize the data generated by the physical devices for visualization and interaction. This section encompasses two key components: the Android App and the Node-RED Dashboard.

### Android App

The Android App serves as a user-friendly interface, allowing users to access and interact with the IoT system. It seamlessly integrates with the data generated by the physical devices, providing real-time updates, control options, and visualizations. The Android App employs robust security measures, including secure user authentication and encrypted communication, to ensure the confidentiality and integrity of the data exchanged between the app and the IoT system.

The Android App is built using the framework **Flutter**.

Using the library *mqtt_client* written in Dart programming language, is it possible to connect every smarthphone to the Iot Project Unical project.

The app is composed by 4 module:
- ***CmdValueHandler.dart***: utils module to manage the command value sent by the android app to the system;
- ***SensorValueHandler.dart***: utils module to manage the sensing value sent by the system to the android app;
- ***main.dart***: main class that manage the app frontend;
- ***MqttHandler.dart***: that manage the MQTT through TLS connection.

#### MqttHandler.dart

The most important part of the Mqtt handler is the connection one.

``` Dart
client = MqttServerClient.withPort(
        'iotprojectunical.me', 'telefono_di_gixs1', 8883);
    client.logging(on: true);
    client.onConnected = onConnected;
    client.onDisconnected = onDisconnected;
    client.onUnsubscribed = onUnsubscribed;
    client.onSubscribed = onSubscribed;
    client.onSubscribeFail = onSubscribeFail;
    client.pongCallback = pong;
    client.keepAlivePeriod = 60;
    client.logging(on: true);
    client.secure = true;
```

Using the flag *client.secure* the flutter app knows that have to use the default *Secure Context*.

The other code written is about the handling of topic and JSon message exchanged.

#### Front End


<p align="center">
  <img src="images/android_app.jpg" alt="Android App Front End" width="300">
</p>

The app shows in two text field the last values of *Air quality* and *Temperature*.

The buttons are disposed to achieve the system actuation.
The one on the left close the alarm generated when the values overcome the thresholds.

The one on the right change the vent state. If the vent is on, put it off and viceversa.

That is done by sending through mqtt the command messages.

### NodeRED Dashboard

The Node-RED Dashboard complements the Android App by providing an additional platform for data visualization and interaction. It offers a customizable and intuitive interface that allows users to monitor and control the IoT system's data flow in real-time. The Node-RED Dashboard incorporates security features, such as secure access controls and encrypted communication channels, to safeguard the data as it flows between the IoT system and the dashboard.

The Northbound section highlights the significance of the Android App and the Node-RED Dashboard in enabling users to effectively visualize and interact with the data generated by the physical devices. By ensuring the security and usability of these applications, the IoT system becomes more accessible and user-friendly, empowering users to make informed decisions based on the insights derived from the data.

#### Back-end

The Dashboard back-end is composed of two part.


The one shown in the [Query and Decrypt](#node-red---query-and-decrypt), for the output from the database.
The second one is shown below and it is a realtime dashboard, showing the last value received from the sensor.

![Node-RED Dashboard backend](images/Node-RED_dashboard.png)

Data already formatted flow into a Non-editable Text Field and is show realtime.
Also a chart of the last hour readings is made realtime.

#### Front-end
To navigate toward the two part of the of the dashboard is build a *side navigation menu*.

![Navigation menu](images/Navigation_menu.png)

Here the user can switch from the realtime dashboard and the dashboard showing the output query from database.

The **realtime dashboard** show the last value received from the sensors and give also a graph of the last values received.

![Real time dashboard](images/realtime_dashboard.png)

The **query database** dashboard part is divided in two part: *user input* and *output*.

![Query db dashboard](images/Dashboard_from_db.png)

The *user input* give the possibility to insert information to query the database.

The *output* part give the output of the query as table and as graph.


## Conclusion

The IoT Security project has successfully addressed the crucial aspects of securing an IoT system, encompassing physical devices, cloud infrastructure, databases, and data visualization applications. By implementing robust security measures across these components, the project has mitigated potential vulnerabilities and ensured the confidentiality, integrity, and availability of the data.

Throughout the project, a comprehensive approach to security has been taken. Physical devices have been protected against tampering and unauthorized access, while secure protocols such as MQTT with TLS have been employed for communication between devices and the cloud. The cloud infrastructure, including the MQTT broker, database, and Node-RED, has been secured with authentication mechanisms, access controls, and encrypted connections. Moreover, the Android App and Node-RED Dashboard have provided intuitive interfaces for data visualization and user interaction while maintaining the necessary security measures.

By deliberately opting for manual data encryption in the database and employing encryption algorithms like DES, the project has demonstrated a deep understanding of encryption techniques and their application within a secure environment.

The successful implementation of this IoT Security project serves as a testament to the importance of addressing security concerns in IoT systems. The project highlights the significance of secure communication, authentication, access controls, and data encryption in safeguarding the privacy and integrity of IoT data.

As technology advances and the IoT landscape continues to expand, it becomes increasingly critical to prioritize security measures. This project has provided valuable insights and recommendations for enhancing the security posture of similar IoT deployments, serving as a foundation for future research and development in the field of IoT Security.

Overall, the project has achieved its objective of implementing robust security measures to protect the IoT system, enabling users to leverage the power of IoT technology while ensuring the confidentiality and integrity of their data.
