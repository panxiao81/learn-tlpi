#!/usr/bin/python3
# (C) Pan Xiao 2022

import argparse
from html.parser import HTMLParser
import os
import shutil
import tarfile
import urllib.request

class MyHTMLParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.flag = False
        self.url = ""
        self.Url = ""
    def handle_data(self, data: str) -> None:
        if data == "Distribution version":
            self.Url = self.url
    def handle_starttag(self, tag: str, attrs) -> None:
        if tag == 'a':
            self.url = self._attr(attrs, "href")
    def _attr(self, attrlist, attrname):
        for attr in attrlist:
            if attr[0] == attrname:
                return attr[1]
        return None

if __name__ == '__main__':
    args = argparse.ArgumentParser(description="Download tlpi source code from man7.org")
    args.add_argument("-s", "--save", help="Don't delete tarball automatically", action="store_true")
    args.add_argument("-f", "--force", help="Force download the source code.", action="store_true")
    argv = args.parse_args()
    if os.path.exists("tlpi-dist"):
        if not argv.force:
            print("The source code exists, exit...")
            exit(0)
        else:
            print("The source code exists, redownload it...")
            shutil.rmtree("tlpi-dest", ignore_errors=True)
    web = "https://man7.org/tlpi/code/"
    with urllib.request.urlopen(web) as response:
        html = response.read()
    parser = MyHTMLParser()
    parser.feed(str(html))
    if not os.path.exists(os.path.basename(parser.Url)):
        print("Downloading ", os.path.basename(parser.Url))
        urllib.request.urlretrieve(web + parser.Url, os.path.basename(parser.Url))
    print("Download complete, unpack it...")
    # unpack and delete the tar file
    with tarfile.open(os.path.basename(parser.Url)) as f:
        f.extractall()
        f.close()
    if not argv.save:
        print("Delete ", os.path.basename(parser.Url))
        os.remove(os.path.basename(parser.Url))