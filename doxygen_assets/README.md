# Doxygen theme assets

This directory contains the repository-local theme layer for the generated
Doxygen HTML documentation.

## Design goals

- keep the documentation readable and maintainable
- improve spacing, typography, navigation emphasis, and code block styling
- avoid fragile full-template overrides when a stylesheet is sufficient

## Wired through

The stylesheet is enabled via `HTML_EXTRA_STYLESHEET` in `Doxyfile`.
