/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"

#include "fboss/agent/FbossError.h"

#include <folly/FileUtil.h>
#include <folly/json.h>

#include <boost/algorithm/string.hpp>
#include <re2/re2.h>

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "fab1/19/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                2,
                3,
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "2": {
        "mapping": {
          "id": 2,
          "name": "fab1/19/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "3": {
        "mapping": {
          "id": 3,
          "name": "fab1/19/3",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "4": {
        "mapping": {
          "id": 4,
          "name": "fab1/19/4",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "FC0",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC0",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "5": {
        "mapping": {
          "id": 5,
          "name": "fab1/17/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                6
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "6": {
        "mapping": {
          "id": 6,
          "name": "fab1/17/2",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "7": {
        "mapping": {
          "id": 7,
          "name": "fab1/17/3",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                8
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "8": {
        "mapping": {
          "id": 8,
          "name": "fab1/17/4",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "FC1",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC1",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "fab1/18/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                10
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                10,
                11,
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "10": {
        "mapping": {
          "id": 10,
          "name": "fab1/18/2",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "11": {
        "mapping": {
          "id": 11,
          "name": "fab1/18/3",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "12": {
        "mapping": {
          "id": 12,
          "name": "fab1/18/4",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "FC2",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "13": {
        "mapping": {
          "id": 13,
          "name": "fab1/20/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                14,
                15,
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "14": {
        "mapping": {
          "id": 14,
          "name": "fab1/20/2",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "15": {
        "mapping": {
          "id": 15,
          "name": "fab1/20/3",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                16
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "16": {
        "mapping": {
          "id": 16,
          "name": "fab1/20/4",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "FC3",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "17": {
        "mapping": {
          "id": 17,
          "name": "fab1/32/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                18
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                18,
                19,
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "18": {
        "mapping": {
          "id": 18,
          "name": "fab1/32/2",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "19": {
        "mapping": {
          "id": 19,
          "name": "fab1/32/3",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                20
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "20": {
        "mapping": {
          "id": 20,
          "name": "fab1/32/4",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "FC4",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC4",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "21": {
        "mapping": {
          "id": 21,
          "name": "fab1/30/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                22
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                22,
                23,
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "22": {
        "mapping": {
          "id": 22,
          "name": "fab1/30/2",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "23": {
        "mapping": {
          "id": 23,
          "name": "fab1/30/3",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                24
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "24": {
        "mapping": {
          "id": 24,
          "name": "fab1/30/4",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "FC5",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC5",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "25": {
        "mapping": {
          "id": 25,
          "name": "fab1/31/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                26
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                26,
                27,
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "26": {
        "mapping": {
          "id": 26,
          "name": "fab1/31/2",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "27": {
        "mapping": {
          "id": 27,
          "name": "fab1/31/3",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                28
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "28": {
        "mapping": {
          "id": 28,
          "name": "fab1/31/4",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "FC6",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "29": {
        "mapping": {
          "id": 29,
          "name": "fab1/27/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                30
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                30,
                31,
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "30": {
        "mapping": {
          "id": 30,
          "name": "fab1/27/2",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "31": {
        "mapping": {
          "id": 31,
          "name": "fab1/27/3",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                32
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "32": {
        "mapping": {
          "id": 32,
          "name": "fab1/27/4",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "FC7",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "34": {
        "mapping": {
          "id": 34,
          "name": "fab1/29/1",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                35
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                35,
                36,
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "35": {
        "mapping": {
          "id": 35,
          "name": "fab1/29/2",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "36": {
        "mapping": {
          "id": 36,
          "name": "fab1/29/3",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                37
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "37": {
        "mapping": {
          "id": 37,
          "name": "fab1/29/4",
          "controllingPort": 34,
          "pins": [
            {
              "a": {
                "chip": "FC8",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC8",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "38": {
        "mapping": {
          "id": 38,
          "name": "fab1/26/1",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                39
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                39,
                40,
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "39": {
        "mapping": {
          "id": 39,
          "name": "fab1/26/2",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "40": {
        "mapping": {
          "id": 40,
          "name": "fab1/26/3",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                41
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "fab1/26/4",
          "controllingPort": 38,
          "pins": [
            {
              "a": {
                "chip": "FC9",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC9",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "42": {
        "mapping": {
          "id": 42,
          "name": "fab1/25/1",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                43
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                43,
                44,
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "43": {
        "mapping": {
          "id": 43,
          "name": "fab1/25/2",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "44": {
        "mapping": {
          "id": 44,
          "name": "fab1/25/3",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "45": {
        "mapping": {
          "id": 45,
          "name": "fab1/25/4",
          "controllingPort": 42,
          "pins": [
            {
              "a": {
                "chip": "FC10",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "46": {
        "mapping": {
          "id": 46,
          "name": "fab1/28/1",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                47,
                48,
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "47": {
        "mapping": {
          "id": 47,
          "name": "fab1/28/2",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "48": {
        "mapping": {
          "id": 48,
          "name": "fab1/28/3",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                49
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "fab1/28/4",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "FC11",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "50": {
        "mapping": {
          "id": 50,
          "name": "fab1/8/1",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                51
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                51,
                52,
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "51": {
        "mapping": {
          "id": 51,
          "name": "fab1/8/2",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "52": {
        "mapping": {
          "id": 52,
          "name": "fab1/8/3",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                53
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "53": {
        "mapping": {
          "id": 53,
          "name": "fab1/8/4",
          "controllingPort": 50,
          "pins": [
            {
              "a": {
                "chip": "FC12",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC12",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "54": {
        "mapping": {
          "id": 54,
          "name": "fab1/7/1",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                55
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                55,
                56,
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "55": {
        "mapping": {
          "id": 55,
          "name": "fab1/7/2",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "56": {
        "mapping": {
          "id": 56,
          "name": "fab1/7/3",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                57
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "fab1/7/4",
          "controllingPort": 54,
          "pins": [
            {
              "a": {
                "chip": "FC13",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC13",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "58": {
        "mapping": {
          "id": 58,
          "name": "fab1/6/1",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                59
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                59,
                60,
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "59": {
        "mapping": {
          "id": 59,
          "name": "fab1/6/2",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "60": {
        "mapping": {
          "id": 60,
          "name": "fab1/6/3",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                61
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "61": {
        "mapping": {
          "id": 61,
          "name": "fab1/6/4",
          "controllingPort": 58,
          "pins": [
            {
              "a": {
                "chip": "FC14",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "62": {
        "mapping": {
          "id": 62,
          "name": "fab1/5/1",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                63
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                63,
                64,
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "63": {
        "mapping": {
          "id": 63,
          "name": "fab1/5/2",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "64": {
        "mapping": {
          "id": 64,
          "name": "fab1/5/3",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                65
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "65": {
        "mapping": {
          "id": 65,
          "name": "fab1/5/4",
          "controllingPort": 62,
          "pins": [
            {
              "a": {
                "chip": "FC15",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "68": {
        "mapping": {
          "id": 68,
          "name": "fab1/3/1",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                69,
                70,
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "69": {
        "mapping": {
          "id": 69,
          "name": "fab1/3/2",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "70": {
        "mapping": {
          "id": 70,
          "name": "fab1/3/3",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                71
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "71": {
        "mapping": {
          "id": 71,
          "name": "fab1/3/4",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "FC16",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC16",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "72": {
        "mapping": {
          "id": 72,
          "name": "fab1/1/1",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                73
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                73,
                74,
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "73": {
        "mapping": {
          "id": 73,
          "name": "fab1/1/2",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "74": {
        "mapping": {
          "id": 74,
          "name": "fab1/1/3",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                75
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "75": {
        "mapping": {
          "id": 75,
          "name": "fab1/1/4",
          "controllingPort": 72,
          "pins": [
            {
              "a": {
                "chip": "FC17",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC17",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "76": {
        "mapping": {
          "id": 76,
          "name": "fab1/2/1",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                77
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                77,
                78,
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "77": {
        "mapping": {
          "id": 77,
          "name": "fab1/2/2",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "78": {
        "mapping": {
          "id": 78,
          "name": "fab1/2/3",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                79
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "79": {
        "mapping": {
          "id": 79,
          "name": "fab1/2/4",
          "controllingPort": 76,
          "pins": [
            {
              "a": {
                "chip": "FC18",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "80": {
        "mapping": {
          "id": 80,
          "name": "fab1/4/1",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                81
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                81,
                82,
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "81": {
        "mapping": {
          "id": 81,
          "name": "fab1/4/2",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "82": {
        "mapping": {
          "id": 82,
          "name": "fab1/4/3",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                83
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "83": {
        "mapping": {
          "id": 83,
          "name": "fab1/4/4",
          "controllingPort": 80,
          "pins": [
            {
              "a": {
                "chip": "FC19",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "84": {
        "mapping": {
          "id": 84,
          "name": "fab1/15/1",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                85
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                85,
                86,
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "85": {
        "mapping": {
          "id": 85,
          "name": "fab1/15/2",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "86": {
        "mapping": {
          "id": 86,
          "name": "fab1/15/3",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                87
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "87": {
        "mapping": {
          "id": 87,
          "name": "fab1/15/4",
          "controllingPort": 84,
          "pins": [
            {
              "a": {
                "chip": "FC20",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC20",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "88": {
        "mapping": {
          "id": 88,
          "name": "fab1/16/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                89,
                90,
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "89": {
        "mapping": {
          "id": 89,
          "name": "fab1/16/2",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "90": {
        "mapping": {
          "id": 90,
          "name": "fab1/16/3",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "91": {
        "mapping": {
          "id": 91,
          "name": "fab1/16/4",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "FC21",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC21",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "92": {
        "mapping": {
          "id": 92,
          "name": "fab1/14/1",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                93
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "93": {
        "mapping": {
          "id": 93,
          "name": "fab1/14/2",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "94": {
        "mapping": {
          "id": 94,
          "name": "fab1/14/3",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                95
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "95": {
        "mapping": {
          "id": 95,
          "name": "fab1/14/4",
          "controllingPort": 92,
          "pins": [
            {
              "a": {
                "chip": "FC22",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "96": {
        "mapping": {
          "id": 96,
          "name": "fab1/13/1",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                97
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                97,
                98,
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "97": {
        "mapping": {
          "id": 97,
          "name": "fab1/13/2",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "98": {
        "mapping": {
          "id": 98,
          "name": "fab1/13/3",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                99
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "99": {
        "mapping": {
          "id": 99,
          "name": "fab1/13/4",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "FC23",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "102": {
        "mapping": {
          "id": 102,
          "name": "fab1/11/1",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                103
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                103,
                104,
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "103": {
        "mapping": {
          "id": 103,
          "name": "fab1/11/2",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "104": {
        "mapping": {
          "id": 104,
          "name": "fab1/11/3",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                105
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "fab1/11/4",
          "controllingPort": 102,
          "pins": [
            {
              "a": {
                "chip": "FC24",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC24",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "106": {
        "mapping": {
          "id": 106,
          "name": "fab1/10/1",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                107
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                107,
                108,
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "107": {
        "mapping": {
          "id": 107,
          "name": "fab1/10/2",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "108": {
        "mapping": {
          "id": 108,
          "name": "fab1/10/3",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                109
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "109": {
        "mapping": {
          "id": 109,
          "name": "fab1/10/4",
          "controllingPort": 106,
          "pins": [
            {
              "a": {
                "chip": "FC25",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC25",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "110": {
        "mapping": {
          "id": 110,
          "name": "fab1/9/1",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                111,
                112,
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "111": {
        "mapping": {
          "id": 111,
          "name": "fab1/9/2",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "112": {
        "mapping": {
          "id": 112,
          "name": "fab1/9/3",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "113": {
        "mapping": {
          "id": 113,
          "name": "fab1/9/4",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "FC26",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "114": {
        "mapping": {
          "id": 114,
          "name": "fab1/12/1",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                115
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "115": {
        "mapping": {
          "id": 115,
          "name": "fab1/12/2",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "116": {
        "mapping": {
          "id": 116,
          "name": "fab1/12/3",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                117
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "117": {
        "mapping": {
          "id": 117,
          "name": "fab1/12/4",
          "controllingPort": 114,
          "pins": [
            {
              "a": {
                "chip": "FC27",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "118": {
        "mapping": {
          "id": 118,
          "name": "fab1/22/1",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                119
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                119,
                120,
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "119": {
        "mapping": {
          "id": 119,
          "name": "fab1/22/2",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "120": {
        "mapping": {
          "id": 120,
          "name": "fab1/22/3",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                121
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "fab1/22/4",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "FC28",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC28",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "122": {
        "mapping": {
          "id": 122,
          "name": "fab1/23/1",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                123
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                123,
                124,
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "123": {
        "mapping": {
          "id": 123,
          "name": "fab1/23/2",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "124": {
        "mapping": {
          "id": 124,
          "name": "fab1/23/3",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                125
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "125": {
        "mapping": {
          "id": 125,
          "name": "fab1/23/4",
          "controllingPort": 122,
          "pins": [
            {
              "a": {
                "chip": "FC29",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC29",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "126": {
        "mapping": {
          "id": 126,
          "name": "fab1/24/1",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                127
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                127,
                128,
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "127": {
        "mapping": {
          "id": 127,
          "name": "fab1/24/2",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "128": {
        "mapping": {
          "id": 128,
          "name": "fab1/24/3",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                129
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "129": {
        "mapping": {
          "id": 129,
          "name": "fab1/24/4",
          "controllingPort": 126,
          "pins": [
            {
              "a": {
                "chip": "FC30",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "130": {
        "mapping": {
          "id": 130,
          "name": "fab1/21/1",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 0
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                131
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ]
              }
          },
          "32": {
              "subsumedPorts": [
                131,
                132,
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "131": {
        "mapping": {
          "id": 131,
          "name": "fab1/21/2",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 1
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 1
                    }
                  }
                ]
              }
          }
        }
    },
    "132": {
        "mapping": {
          "id": 132,
          "name": "fab1/21/3",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 2
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  }
                ]
              }
          },
          "19": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "133": {
        "mapping": {
          "id": 133,
          "name": "fab1/21/4",
          "controllingPort": 130,
          "pins": [
            {
              "a": {
                "chip": "FC31",
                "lane": 3
              }
            }
          ]
        },
        "supportedProfiles": {
          "14": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "FC31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "supportedProfiles": {
    "12": {
        "speed": 10000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 41,
          "interfaceType": 41
        }
    },
    "14": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 10,
          "interfaceType": 10
        }
    },
    "18": {
        "speed": 40000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 40,
          "interfaceType": 40
        }
    },
    "19": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 11,
          "interfaceType": 11
        }
    },
    "28": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 91,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
    },
    "30": {
        "speed": 25000,
        "iphy": {
          "numLanes": 1,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
    },
    "31": {
        "speed": 50000,
        "iphy": {
          "numLanes": 2,
          "modulation": 1,
          "fec": 1,
          "medium": 2,
          "interfaceMode": 30,
          "interfaceType": 30
        }
    },
    "32": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 1,
          "medium": 1,
          "interfaceMode": 12,
          "interfaceType": 12
        }
    }
  },
  "chips": [
    {
      "name": "FC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "FC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "FC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "FC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "FC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "FC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "FC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "FC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "FC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "FC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "FC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "FC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "FC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "FC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "FC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "FC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "FC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "FC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "FC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "FC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "FC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "FC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "FC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "FC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "FC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "FC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "FC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "FC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "FC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "FC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "FC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "FC4",
      "type": 1,
      "physicalID": 4
    }
  ]
}
)";

constexpr auto kFabricCardNameRegex = "fc(\\d+)";
constexpr auto kDefaultFCName = "fc001";
constexpr auto kLCName = "lc_name";

std::string updatePlatformMappingStr(const std::string& lcName) {
  int cardID = 0;
  re2::RE2 cardIDRe(kFabricCardNameRegex);
  if (!re2::RE2::FullMatch(lcName, cardIDRe, &cardID)) {
    throw facebook::fboss::FbossError("Invalid fabric card name:", lcName);
  }
  std::string tmpStr = kJsonPlatformMappingStr;
  boost::replace_all(tmpStr, "fab1", folly::to<std::string>("fab", cardID));
  return tmpStr;
}
} // namespace

namespace facebook {
namespace fboss {
GalaxyFCPlatformMapping::GalaxyFCPlatformMapping(
    const std::string& linecardName)
    : PlatformMapping(updatePlatformMappingStr(linecardName)) {}

std::string GalaxyFCPlatformMapping::getFabriccardName() {
  std::string netwhoamiStr;
  if (!folly::readFile(FLAGS_netwhoami.data(), netwhoamiStr)) {
    return kDefaultFCName;
  }

  auto netwhoamiDynamic = folly::parseJson(netwhoamiStr);
  if (netwhoamiDynamic.find(kLCName) == netwhoamiDynamic.items().end()) {
    return kDefaultFCName;
  }
  return netwhoamiDynamic[kLCName].asString();
}
} // namespace fboss
} // namespace facebook
