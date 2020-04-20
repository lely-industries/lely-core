from setuptools import setup, find_packages

setup(
    name="dcf-tools",
    version="2.1.0",
    packages=find_packages(),
    entry_points={"console_scripts": ["dcfchk = dcf.cli:main"]},
    author="J. S. Seldenthuis",
    author_email="jseldenthuis@lely.com",
    description="Tools to generate and manipulate DCF files",
    url="https://gitlab.com/lely_industries/lely-core",
    classifiers=["License :: OSI Approved :: Apache Software License"],
)
