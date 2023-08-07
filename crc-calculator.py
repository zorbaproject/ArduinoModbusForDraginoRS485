#!/usr/bin/python3

#Translated from C++:
#https://ctlsys.com/support/how_to_compute_the_modbus_rtu_message_crc/

def modbusCRC(msg:str) -> int:
    crc = 0xFFFF
    for n in range(len(msg)):
        crc ^= msg[n]
        for i in range(8):
            if crc & 0x0001:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc

print("Enter message in HEX")
hexmsg = input()
msg = bytes.fromhex(hexmsg.replace(" ",""))
crc = modbusCRC(msg)
ba = crc.to_bytes(2, byteorder='little')
print(hex(ba[0])[2:].upper().zfill(2) + " " + hex(ba[1])[2:].upper().zfill(2))
