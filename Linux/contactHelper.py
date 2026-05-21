#!/usr/bin/python3
import csv
import os

filename = "email.txt"
filename_output = f"{os.path.splitext(filename)[0]}.csv"

with open(filename, 'r') as f:
    content = f.read()

emails = content.split(',')

cleaned_emails = []
    
for e in emails:
    print(e)
    if e == "" or e == None:
        continue
    email = e.split("<")[1].replace(">","\r\n")
    cleaned_emails.append(email.strip())

cleaned_emails = list(set(cleaned_emails))
    
with open(filename_output, 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['email'])
    for email in cleaned_emails:
        writer.writerow([email])
