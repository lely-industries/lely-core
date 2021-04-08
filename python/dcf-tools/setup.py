from setuptools import setup, find_packages

setup(
    name="dcf-tools",
    version="2.4.0",
    packages=find_packages(),
    entry_points={
        "console_scripts": [
            "dcf2dev = dcf2dev.cli:main",
            "dcfchk = dcf.cli:main",
            "dcfgen = dcfgen.cli:main",
        ]
    },
    install_requires=["PyYAML>=3.08", "empy>=3.3.2"],
    package_data={"": ["data/*"]},
    author="J. S. Seldenthuis",
    author_email="jseldenthuis@lely.com",
    description="Tools to generate and manipulate DCF files",
    url="https://gitlab.com/lely_industries/lely-core",
    classifiers=["License :: OSI Approved :: Apache Software License"],
)
