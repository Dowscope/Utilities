#!/usr/bin/python3

import requests
from bs4 import BeautifulSoup

def get_user_search_string():
    return input("Enter a search string: ").strip()

def get_users_selection():
    return int(input("Enter the number or 0 for exit: "))

def get_searchresults(data):
    table = data.find('table', id='searchResult')
    if not table:
        print("\033[31mTable ID 'searchResult' not found\033[0m")
        return []
    
    rows = table.find_all('tr')[1:]

    results = []

    for idx, row in enumerate(rows, 1):
        columns = row.find_all('td')
        if len(columns) < 2:
            continue

        title_link = columns[1].find('div', class_='detName')
        
        name = None
        if title_link and title_link.a:
            name = title_link.a.text.strip()
        else:
            continue

        magnet_link = None
        for a_tag in columns[1].find_all('a'):
            href = a_tag.get('href', '')
            if href.startswith('magnet:'):
                magnet_link = href
                break

        if name and magnet_link:
            results.append({
                'index': idx,
                'name': name,
                'magnet': magnet_link
            })

    return results

def fetch_html(url):
    try:
        response = requests.get(url)
        response.raise_for_status()
        soup = BeautifulSoup(response.text, 'html.parser')
        return soup
    except requests.exceptions.RequestException as e:
        print(f"\033[31mFailed to make request: {e}\033[0m")

def main():
    print ("\033[32m*****************************************")
    print ("*          File Finder Script")
    print ("*****************************************\033[0m")
    
    search = get_user_search_string()

    url = f"https://thepiratebay0.org/search/{search}/1/99/0"
    data = fetch_html(url)
    results = get_searchresults(data)
    
    for r in results:
        print (f"{r['index']}. {r['name']}")

    selection = get_users_selection()
    if selection == 0:
        print("Good Bye!")
        return

    selected = next((item for item in results if item['index'] == selection), None)

    if selected:
        print(f"\n\033[32mYou selected:\033[0m {selected['name']}")
        print(f"\033[33mMagnet Link:\033[0m {selected['magnet']}")
    else:
        print("\033[31mInvalid selection.\033[0m")

if __name__ == "__main__":
    main()
