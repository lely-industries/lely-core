#!/bin/env python3

import requests
import html

class Sonar:

    def __init__(self, token):
        self.token = token
        self.auth = (token, '')
        self.url = "https://sonarcloud.io/api"

    def download_issues(self, organization, project, resolved):
        result = []
        page = 1

        while True:
            args = {
                "organization": organization,
                "project": project,
                "resolutions": "FALSE-POSITIVE,WONTFIX" if resolved else "",
                "resolved": "yes" if resolved else "no",
                "additionalFields": "comments",
                "s": "SEVERITY",
                "asc": "no",
                "ps": 500,
                "p": page,
            }
            page += 1

            response = requests.get(f"{self.url}/issues/search", args,
                                    auth=self.auth)
            if response.status_code != 200:
                raise Exception(f"Unable to download Quality Profile rules "
                                f"{response.status_code} {response.content}")

            result += response.json()["issues"]

            if args["ps"] * (page - 1) > int(response.json()["paging"]["total"]):
                break

        return result


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

    def dump_issue(self, idx, issue):
        self.output.writelines([
            f"<h1>{idx}. {html.escape(issue['message'])}</h1>",
            f"<h2>{issue['component'].replace(issue['project']+':','')}:{issue.get('line',0)}</h2>\n",
            f"Severity: {issue['severity']}<br/>\n",
            f"Resolution: {issue.get('resolution', 'UNRESOLVED')}\n",
        ])
        if len(issue['comments']) > 0:
            self.output.writelines([
                f"<h3>Comments</h3>",
            ])
        for comment in issue['comments']:
            self.output.writelines([
                f"{comment['login']}: {comment['htmlText']} ({comment['createdAt']})<br/>"
                ])
        self.output.writelines([
            "<br/><br/><br/>\n",
        ])


def dump_issues(token, organization, project):
    sonar = Sonar(token)
    issues = sonar.download_issues(organization, project, resolved=False)
    issues += sonar.download_issues(organization, project, resolved=True)

    with HtmlDumper(open("sonar-issues.html", "w")) as html:
        for idx, issue in enumerate(issues):
            html.dump_issue(idx + 1, issue)


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 4:
        print(f"Usage {sys.argv[0]} <token> <organization> <project>")
        sys.exit(-1)

    dump_issues(sys.argv[1], sys.argv[2], sys.argv[3])
