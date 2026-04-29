#!/usr/bin/env python3

# pyre-strict

import os
import unittest
from unittest.mock import MagicMock, patch

from fboss.lib.platform_mapping_v2.facebook import gen


class IsContinuousScheduleTest(unittest.TestCase):
    @patch.dict(os.environ, {"SANDCASTLE_SCHEDULE_TYPE": "continuous"}, clear=False)
    def test_returns_true_for_continuous(self) -> None:
        self.assertTrue(gen._is_continuous_schedule())

    @patch.dict(os.environ, {"SANDCASTLE_SCHEDULE_TYPE": "diff"}, clear=False)
    def test_returns_false_for_diff(self) -> None:
        self.assertFalse(gen._is_continuous_schedule())

    @patch.dict(os.environ, {"SANDCASTLE_SCHEDULE_TYPE": "landcastle"}, clear=False)
    def test_returns_false_for_landcastle(self) -> None:
        self.assertFalse(gen._is_continuous_schedule())

    def test_returns_false_when_unset(self) -> None:
        env = {k: v for k, v in os.environ.items() if k != "SANDCASTLE_SCHEDULE_TYPE"}
        with patch.dict(os.environ, env, clear=True):
            self.assertFalse(gen._is_continuous_schedule())


class GenerateDiffTitleTest(unittest.TestCase):
    def test_no_diff_returns_base_title(self) -> None:
        title = gen._generate_diff_title("")
        self.assertEqual(
            title,
            "[platform-mapping-sync] Syncing Platform Mapping Changes to Configerator",
        )
        self.assertNotIn("from", title)

    def test_with_diff_includes_diff_number(self) -> None:
        title = gen._generate_diff_title("D102639093")
        self.assertIn("from D102639093", title)


class GenerateDiffDescriptionTest(unittest.TestCase):
    def test_continuous_description_mentions_oncall_and_continuous(self) -> None:
        summary = gen._generate_diff_description("", is_continuous=True)
        self.assertIn("fboss_optics_phy oncall", summary)
        self.assertIn("continuous trunk-validation", summary)
        self.assertNotIn("Depends on", summary)
        self.assertNotIn("manually running", summary)

    def test_diff_attributed_description_includes_dependency(self) -> None:
        summary = gen._generate_diff_description("D102639093", is_continuous=False)
        self.assertIn("Depends on D102639093", summary)

    def test_manual_description_when_neither_diff_nor_continuous(self) -> None:
        summary = gen._generate_diff_description("", is_continuous=False)
        self.assertIn("manually running", summary)
        self.assertNotIn("Depends on", summary)


class CommitWithConfigoContinuousTest(unittest.TestCase):
    @patch.dict(os.environ, {"SANDCASTLE_SCHEDULE_TYPE": "continuous"}, clear=False)
    @patch("fboss.lib.platform_mapping_v2.facebook.gen.ConfigoClient")
    @patch("fboss.lib.platform_mapping_v2.facebook.gen._get_curr_diff_author")
    @patch("fboss.lib.platform_mapping_v2.facebook.gen._get_curr_diff")
    def test_continuous_skips_attribution_and_uses_oncall_only(
        self,
        mock_get_curr_diff: MagicMock,
        mock_get_curr_diff_author: MagicMock,
        mock_configo_client: MagicMock,
    ) -> None:
        # If anything tries to fetch the current diff or author it would return
        # a draft diff number — the very bug this change fixes. Make these very
        # visible if they are called.
        mock_get_curr_diff.return_value = "D999999999"
        mock_get_curr_diff_author.return_value = "wrong-trunk-author"

        gen.commit_with_configo(
            cconf_files={},
            add_fbcode_dependency=True,
            manual_configerator_diff="",
        )

        # On continuous, _get_curr_diff and _get_curr_diff_author MUST NOT be
        # called — otherwise we'd attribute the autogen to whatever happened to
        # be at trunk tip.
        mock_get_curr_diff.assert_not_called()
        mock_get_curr_diff_author.assert_not_called()

        mutation = mock_configo_client.return_value.sandcastle_safe_transaction.return_value.prepare_mutation_request.return_value.add_objects.return_value.set_author.return_value.set_commit_message.return_value.set_gen_new_thrift_imports.return_value.prepare.return_value
        mutation.review.assert_called_once()
        kwargs = mutation.review.call_args.kwargs
        # The fboss_optics_phy oncall is the sole fbcode-side reviewer (no false
        # attribution to whichever diff happened to be at trunk tip).
        self.assertEqual(kwargs["reviewers"], ["fboss_optics_phy"])
        # Continuous-generated diffs are real automation runs and should auto-publish
        # when CI is ready and auto-land on acceptance, just like diff-scheduled runs.
        self.assertIn("publish_when_ready", kwargs["tags"])
        self.assertIn("accept2ship", kwargs["tags"])
        self.assertIn("fboss_platform_mapping_sync", kwargs["tags"])
        # No spurious diff attribution at the mutation level.
        self.assertNotIn("phabricator_diff_number", kwargs)

    @patch.dict(os.environ, {"SANDCASTLE_SCHEDULE_TYPE": "diff"}, clear=False)
    @patch("fboss.lib.platform_mapping_v2.facebook.gen.ConfigoClient")
    @patch("fboss.lib.platform_mapping_v2.facebook.gen._get_dependent_diffs")
    @patch("fboss.lib.platform_mapping_v2.facebook.gen._get_curr_diff_author")
    @patch("fboss.lib.platform_mapping_v2.facebook.gen._get_curr_diff")
    def test_diff_schedule_keeps_attribution(
        self,
        mock_get_curr_diff: MagicMock,
        mock_get_curr_diff_author: MagicMock,
        mock_get_dependent_diffs: MagicMock,
        mock_configo_client: MagicMock,
    ) -> None:
        # Sanity check: the diff-schedule code path is unchanged. The trunk
        # diff author IS added as a reviewer alongside the oncall, and the
        # publish/accept tags ARE applied.
        mock_get_curr_diff.return_value = "D102639093"
        mock_get_curr_diff_author.return_value = "trunk-diff-author"
        mock_get_dependent_diffs.return_value = []

        gen.commit_with_configo(
            cconf_files={},
            add_fbcode_dependency=True,
            manual_configerator_diff="",
        )

        mutation = mock_configo_client.return_value.sandcastle_safe_transaction.return_value.prepare_mutation_request.return_value.add_objects.return_value.set_author.return_value.set_commit_message.return_value.set_gen_new_thrift_imports.return_value.prepare.return_value
        mutation.review.assert_called_once()
        kwargs = mutation.review.call_args.kwargs
        self.assertIn("trunk-diff-author", kwargs["reviewers"])
        self.assertIn("fboss_optics_phy", kwargs["reviewers"])
        self.assertIn("publish_when_ready", kwargs["tags"])
        self.assertIn("accept2ship", kwargs["tags"])


if __name__ == "__main__":
    unittest.main()
