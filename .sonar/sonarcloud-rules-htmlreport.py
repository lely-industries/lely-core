#!/bin/env python3

import requests

class Sonar:

    def __init__(self, token):
        self.token = token
        self.auth = (token, '')
        self.url = "https://sonarcloud.io/api"

    def find_quality_profile(self, organization, project):
        args = {
            "organization": organization,
            "project": project,
            "language": "c",
        }

        response = requests.get(f"{self.url}/qualityprofiles/search", args,
                                auth=self.auth)
        if response.status_code != 200:
            raise Exception(f"Unable to find Quality Profile "
                            f"{response.status_code} {response.content}")

        json = response.json()["profiles"]

        if len(json) != 1:
            raise Exception(f"Bad number of Quality Profiles {len(json)}")

        return json[0]

    def download_rules(self, qualityprofile_key):
        args = {
            "qprofile": qualityprofile_key,
            "activation": "true",
            "ps": 500,
        }

        response = requests.get(f"{self.url}/rules/search", args,
                                auth=self.auth)
        if response.status_code != 200:
            raise Exception(f"Unable to download Quality Profile rules "
                            f"{response.status_code} {response.content}")

        return response.json()["rules"]


class HtmlDumper:

    def __init__(self, output):
        self.output = output

    def __enter__(self):
        self.output.writelines([
            "<html>\n",
            "<body>\n",
        ])
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.output.writelines([
            "</body>\n",
            "</html>\n",
        ])
        self.output.close()

    def dump_rule(self, idx, rule):
        self.output.writelines([
            f"<h1>{idx}. {rule['name']}</h1>",
            f"<br/>Severity: {rule['severity']}<br/><br/>\n",
            rule['htmlDesc'],
            "<br/><br/><br/>\n",
        ])


def dump_rules(token, organization, project):
    sonar = Sonar(token)
    qprofile = sonar.find_quality_profile(organization, project)
    rules = sonar.download_rules(qprofile["key"])

    with HtmlDumper(open("sonar-rules.html", "w")) as html:
        for idx, rule in enumerate(rules):
            html.dump_rule(idx + 1, rule)


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 4:
        print(f"Usage {sys.argv[0]} <token> <organization> <project>")
        sys.exit(-1)

    dump_rules(sys.argv[1], sys.argv[2], sys.argv[3])
