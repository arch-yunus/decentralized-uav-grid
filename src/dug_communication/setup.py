from setuptools import setup

package_name = "dug_communication"

setup(
    name=package_name,
    version="1.0.0",
    packages=[package_name],
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="Bahattin Yunus",
    maintainer_email="bahat@example.com",
    description="Mesh communication layer for DUG",
    license="MIT",
    tests_require=["pytest"],
    entry_points={
        "console_scripts": [
            "mesh_monitor = dug_communication.mesh_monitor:main"
        ],
    },
)

