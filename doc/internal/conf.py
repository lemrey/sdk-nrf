# Internal documentation build configuration file

from pathlib import Path
import sys
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

DOC_INTERNAL_BASE = utils.get_projdir("internal")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# pylint: disable=undefined-variable

# General ----------------------------------------------------------------------

# Import nrfx configuration, override as needed later
conf = eval_config_file(str(DOC_INTERNAL_BASE / "doc" / "sphinx" / "conf.py"), tags)
locals().update(conf)

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))
extensions.extend(["ncs_cache", "zephyr.external_content"])
exclude_patterns = ["readme.rst"]

# Options for HTML output ------------------------------------------------------

html_theme_options = {"docset": "internal", "docsets": utils.ALL_DOCSETS}

# Options for external_content -------------------------------------------------

external_content_contents = [
    (DOC_INTERNAL_BASE / "doc", "**/*"),
]

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "internal"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"

# pylint: enable=undefined-variable


def setup(app):
    utils.add_google_analytics(app)
