
/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/sandia/SandiaPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/1/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/1",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "5": {
        "mapping": {
          "id": 5,
          "name": "eth1/2/1",
          "controllingPort": 5,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/2",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "eth1/3/1",
          "controllingPort": 9,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/3",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "13": {
        "mapping": {
          "id": 13,
          "name": "eth1/4/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "IFG0",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG0",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY0",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY0",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/4",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "17": {
        "mapping": {
          "id": 17,
          "name": "eth1/5/1",
          "controllingPort": 17,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/5",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "21": {
        "mapping": {
          "id": 21,
          "name": "eth1/6/1",
          "controllingPort": 21,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/6",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "25": {
        "mapping": {
          "id": 25,
          "name": "eth1/7/1",
          "controllingPort": 25,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/7",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "29": {
        "mapping": {
          "id": 29,
          "name": "eth1/8/1",
          "controllingPort": 29,
          "pins": [
            {
              "a": {
                "chip": "IFG1",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG1",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY3",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY3",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/8",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "33": {
        "mapping": {
          "id": 33,
          "name": "eth1/9/1",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/9",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "37": {
        "mapping": {
          "id": 37,
          "name": "eth1/10/1",
          "controllingPort": 37,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/10",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "eth1/11/1",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/11",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "45": {
        "mapping": {
          "id": 45,
          "name": "eth1/12/1",
          "controllingPort": 45,
          "pins": [
            {
              "a": {
                "chip": "IFG2",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG2",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY6",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY6",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/12",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "eth1/13/1",
          "controllingPort": 49,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/13",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "53": {
        "mapping": {
          "id": 53,
          "name": "eth1/14/1",
          "controllingPort": 53,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/14",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "eth1/15/1",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/15",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "61": {
        "mapping": {
          "id": 61,
          "name": "eth1/16/1",
          "controllingPort": 61,
          "pins": [
            {
              "a": {
                "chip": "IFG3",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG3",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY9",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY9",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/16",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "65": {
        "mapping": {
          "id": 65,
          "name": "eth1/17/1",
          "controllingPort": 65,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/17",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "69": {
        "mapping": {
          "id": 69,
          "name": "eth1/18/1",
          "controllingPort": 69,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/18",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "73": {
        "mapping": {
          "id": 73,
          "name": "eth1/19/1",
          "controllingPort": 73,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/19",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "77": {
        "mapping": {
          "id": 77,
          "name": "eth1/20/1",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "IFG4",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG4",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY12",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY12",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/20",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "81": {
        "mapping": {
          "id": 81,
          "name": "eth1/21/1",
          "controllingPort": 81,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/21",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "85": {
        "mapping": {
          "id": 85,
          "name": "eth1/22/1",
          "controllingPort": 85,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/22",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "89": {
        "mapping": {
          "id": 89,
          "name": "eth1/23/1",
          "controllingPort": 89,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/23",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "93": {
        "mapping": {
          "id": 93,
          "name": "eth1/24/1",
          "controllingPort": 93,
          "pins": [
            {
              "a": {
                "chip": "IFG5",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG5",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY15",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY15",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/24",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "97": {
        "mapping": {
          "id": 97,
          "name": "eth1/25/1",
          "controllingPort": 97,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/25",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "101": {
        "mapping": {
          "id": 101,
          "name": "eth1/26/1",
          "controllingPort": 101,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/26",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "eth1/27/1",
          "controllingPort": 105,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/27",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "109": {
        "mapping": {
          "id": 109,
          "name": "eth1/28/1",
          "controllingPort": 109,
          "pins": [
            {
              "a": {
                "chip": "IFG6",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG6",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY18",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY18",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/28",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "113": {
        "mapping": {
          "id": 113,
          "name": "eth1/29/1",
          "controllingPort": 113,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/29",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "117": {
        "mapping": {
          "id": 117,
          "name": "eth1/30/1",
          "controllingPort": 117,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/30",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "eth1/31/1",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/31",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "125": {
        "mapping": {
          "id": 125,
          "name": "eth1/32/1",
          "controllingPort": 125,
          "pins": [
            {
              "a": {
                "chip": "IFG7",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG7",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY21",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY21",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/32",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "129": {
        "mapping": {
          "id": 129,
          "name": "eth1/33/1",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/33",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/33",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/33",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/33",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "133": {
        "mapping": {
          "id": 133,
          "name": "eth1/34/1",
          "controllingPort": 133,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/34",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/34",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/34",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/34",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "137": {
        "mapping": {
          "id": 137,
          "name": "eth1/35/1",
          "controllingPort": 137,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/35",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/35",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/35",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/35",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "141": {
        "mapping": {
          "id": 141,
          "name": "eth1/36/1",
          "controllingPort": 141,
          "pins": [
            {
              "a": {
                "chip": "IFG8",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/36",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/36",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/36",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG8",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY24",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY24",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/36",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "145": {
        "mapping": {
          "id": 145,
          "name": "eth1/37/1",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/37",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/37",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/37",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/37",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "149": {
        "mapping": {
          "id": 149,
          "name": "eth1/38/1",
          "controllingPort": 149,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/38",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/38",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/38",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/38",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "153": {
        "mapping": {
          "id": 153,
          "name": "eth1/39/1",
          "controllingPort": 153,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/39",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/39",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/39",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/39",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "157": {
        "mapping": {
          "id": 157,
          "name": "eth1/40/1",
          "controllingPort": 157,
          "pins": [
            {
              "a": {
                "chip": "IFG9",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/40",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/40",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/40",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG9",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY27",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY27",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/40",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "161": {
        "mapping": {
          "id": 161,
          "name": "eth1/41/1",
          "controllingPort": 161,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/41",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/41",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/41",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/41",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "165": {
        "mapping": {
          "id": 165,
          "name": "eth1/42/1",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/42",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/42",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/42",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/42",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "169": {
        "mapping": {
          "id": 169,
          "name": "eth1/43/1",
          "controllingPort": 169,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/43",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/43",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/43",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/43",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "173": {
        "mapping": {
          "id": 173,
          "name": "eth1/44/1",
          "controllingPort": 173,
          "pins": [
            {
              "a": {
                "chip": "IFG10",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/44",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/44",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/44",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG10",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY20",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY20",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/44",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "177": {
        "mapping": {
          "id": 177,
          "name": "eth1/45/1",
          "controllingPort": 177,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/45",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/45",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/45",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/45",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "181": {
        "mapping": {
          "id": 181,
          "name": "eth1/46/1",
          "controllingPort": 181,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/46",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/46",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/46",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/46",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "185": {
        "mapping": {
          "id": 185,
          "name": "eth1/47/1",
          "controllingPort": 185,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/47",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/47",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/47",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/47",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "189": {
        "mapping": {
          "id": 189,
          "name": "eth1/48/1",
          "controllingPort": 189,
          "pins": [
            {
              "a": {
                "chip": "IFG11",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/48",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/48",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/48",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG11",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY23",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY23",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/48",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "193": {
        "mapping": {
          "id": 193,
          "name": "eth1/49/1",
          "controllingPort": 193,
          "pins": [
            {
              "a": {
                "chip": "IFG12",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/49",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/49",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/49",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/49",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "197": {
        "mapping": {
          "id": 197,
          "name": "eth1/50/1",
          "controllingPort": 197,
          "pins": [
            {
              "a": {
                "chip": "IFG12",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/50",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/50",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/50",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/50",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "201": {
        "mapping": {
          "id": 201,
          "name": "eth1/51/1",
          "controllingPort": 201,
          "pins": [
            {
              "a": {
                "chip": "IFG12",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/51",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/51",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/51",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/51",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "205": {
        "mapping": {
          "id": 205,
          "name": "eth1/52/1",
          "controllingPort": 205,
          "pins": [
            {
              "a": {
                "chip": "IFG12",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/52",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/52",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/52",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG12",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY26",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY26",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/52",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "209": {
        "mapping": {
          "id": 209,
          "name": "eth1/53/1",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "IFG13",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/53",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/53",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/53",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/53",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "213": {
        "mapping": {
          "id": 213,
          "name": "eth1/54/1",
          "controllingPort": 213,
          "pins": [
            {
              "a": {
                "chip": "IFG13",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/54",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/54",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/54",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/54",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "217": {
        "mapping": {
          "id": 217,
          "name": "eth1/55/1",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "IFG13",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/55",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/55",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/55",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/55",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "221": {
        "mapping": {
          "id": 221,
          "name": "eth1/56/1",
          "controllingPort": 221,
          "pins": [
            {
              "a": {
                "chip": "IFG13",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/56",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/56",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/56",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG13",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY29",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY29",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/56",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "225": {
        "mapping": {
          "id": 225,
          "name": "eth1/57/1",
          "controllingPort": 225,
          "pins": [
            {
              "a": {
                "chip": "IFG14",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/57",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/57",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/57",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/57",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "229": {
        "mapping": {
          "id": 229,
          "name": "eth1/58/1",
          "controllingPort": 229,
          "pins": [
            {
              "a": {
                "chip": "IFG14",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/58",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/58",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/58",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/58",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "233": {
        "mapping": {
          "id": 233,
          "name": "eth1/59/1",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "IFG14",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/59",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/59",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/59",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/59",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "237": {
        "mapping": {
          "id": 237,
          "name": "eth1/60/1",
          "controllingPort": 237,
          "pins": [
            {
              "a": {
                "chip": "IFG14",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/60",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/60",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/60",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG14",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY32",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY32",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/60",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "241": {
        "mapping": {
          "id": 241,
          "name": "eth1/61/1",
          "controllingPort": 241,
          "pins": [
            {
              "a": {
                "chip": "IFG15",
                "lane": 0
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 0
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 12
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/61",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 1
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 1
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 14
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/61",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 2
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 2
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 16
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/61",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 3
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 3
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 18
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/61",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "245": {
        "mapping": {
          "id": 245,
          "name": "eth1/62/1",
          "controllingPort": 245,
          "pins": [
            {
              "a": {
                "chip": "IFG15",
                "lane": 4
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 4
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 20
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/62",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 5
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 5
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 22
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/62",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 6
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 6
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 24
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/62",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 7
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 7
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 26
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/62",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "249": {
        "mapping": {
          "id": 249,
          "name": "eth1/63/1",
          "controllingPort": 249,
          "pins": [
            {
              "a": {
                "chip": "IFG15",
                "lane": 8
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 8
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 28
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/63",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 9
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 9
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 30
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/63",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 10
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 10
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 32
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/63",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 11
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 11
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 34
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/63",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    },
    "253": {
        "mapping": {
          "id": 253,
          "name": "eth1/64/1",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "IFG15",
                "lane": 12
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 12
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 36
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/64",
                          "lane": 0
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 13
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 13
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 38
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/64",
                          "lane": 2
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 14
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 14
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 40
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/64",
                          "lane": 4
                        }
                      }
                    }
                  ]
                }
              }
            },
            {
              "a": {
                "chip": "IFG15",
                "lane": 15
              },
              "z": {
                "junction": {
                  "system": {
                    "chip": "XPHY35",
                    "lane": 15
                  },
                  "line": [
                    {
                      "a": {
                        "chip": "XPHY35",
                        "lane": 42
                      },
                      "z": {
                        "end": {
                          "chip": "eth1/64",
                          "lane": 6
                        }
                      }
                    }
                  ]
                }
              }
            }
          ]
        },
        "supportedProfiles": {

        }
    }
  },
  "chips": [
    {
      "name": "IFG0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "IFG1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "IFG2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "IFG3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "IFG4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "IFG5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "IFG6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "IFG7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "IFG8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "IFG9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "IFG10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "IFG11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "IFG12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "IFG13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "IFG14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "IFG15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "eth1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "eth1/17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "eth1/18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "eth1/19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "eth1/20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "eth1/21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "eth1/22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "eth1/23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "eth1/24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "eth1/25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "eth1/26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "eth1/27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "eth1/28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "eth1/29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "eth1/30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "eth1/31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "eth1/32",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "eth1/33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "eth1/34",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "eth1/35",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "eth1/36",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "eth1/37",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "eth1/38",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "eth1/39",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "eth1/40",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "eth1/41",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "eth1/42",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "eth1/43",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "eth1/44",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "eth1/45",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "eth1/46",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "eth1/47",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "eth1/48",
      "type": 3,
      "physicalID": 47
    },
    {
      "name": "eth1/49",
      "type": 3,
      "physicalID": 48
    },
    {
      "name": "eth1/50",
      "type": 3,
      "physicalID": 49
    },
    {
      "name": "eth1/51",
      "type": 3,
      "physicalID": 50
    },
    {
      "name": "eth1/52",
      "type": 3,
      "physicalID": 51
    },
    {
      "name": "eth1/53",
      "type": 3,
      "physicalID": 52
    },
    {
      "name": "eth1/54",
      "type": 3,
      "physicalID": 53
    },
    {
      "name": "eth1/55",
      "type": 3,
      "physicalID": 54
    },
    {
      "name": "eth1/56",
      "type": 3,
      "physicalID": 55
    },
    {
      "name": "eth1/57",
      "type": 3,
      "physicalID": 56
    },
    {
      "name": "eth1/58",
      "type": 3,
      "physicalID": 57
    },
    {
      "name": "eth1/59",
      "type": 3,
      "physicalID": 58
    },
    {
      "name": "eth1/60",
      "type": 3,
      "physicalID": 59
    },
    {
      "name": "eth1/61",
      "type": 3,
      "physicalID": 60
    },
    {
      "name": "eth1/62",
      "type": 3,
      "physicalID": 61
    },
    {
      "name": "eth1/63",
      "type": 3,
      "physicalID": 62
    },
    {
      "name": "eth1/64",
      "type": 3,
      "physicalID": 63
    },
    {
      "name": "XPHY0",
      "type": 2,
      "physicalID": 0
    },
    {
      "name": "XPHY1",
      "type": 2,
      "physicalID": 1
    },
    {
      "name": "XPHY2",
      "type": 2,
      "physicalID": 2
    },
    {
      "name": "XPHY3",
      "type": 2,
      "physicalID": 3
    },
    {
      "name": "XPHY4",
      "type": 2,
      "physicalID": 4
    },
    {
      "name": "XPHY5",
      "type": 2,
      "physicalID": 5
    },
    {
      "name": "XPHY6",
      "type": 2,
      "physicalID": 6
    },
    {
      "name": "XPHY7",
      "type": 2,
      "physicalID": 7
    },
    {
      "name": "XPHY8",
      "type": 2,
      "physicalID": 8
    },
    {
      "name": "XPHY9",
      "type": 2,
      "physicalID": 9
    },
    {
      "name": "XPHY10",
      "type": 2,
      "physicalID": 10
    },
    {
      "name": "XPHY11",
      "type": 2,
      "physicalID": 11
    },
    {
      "name": "XPHY12",
      "type": 2,
      "physicalID": 12
    },
    {
      "name": "XPHY13",
      "type": 2,
      "physicalID": 13
    },
    {
      "name": "XPHY14",
      "type": 2,
      "physicalID": 14
    },
    {
      "name": "XPHY15",
      "type": 2,
      "physicalID": 15
    },
    {
      "name": "XPHY16",
      "type": 2,
      "physicalID": 16
    },
    {
      "name": "XPHY17",
      "type": 2,
      "physicalID": 17
    },
    {
      "name": "XPHY18",
      "type": 2,
      "physicalID": 18
    },
    {
      "name": "XPHY19",
      "type": 2,
      "physicalID": 19
    },
    {
      "name": "XPHY20",
      "type": 2,
      "physicalID": 20
    },
    {
      "name": "XPHY21",
      "type": 2,
      "physicalID": 21
    },
    {
      "name": "XPHY22",
      "type": 2,
      "physicalID": 22
    },
    {
      "name": "XPHY23",
      "type": 2,
      "physicalID": 23
    },
    {
      "name": "XPHY24",
      "type": 2,
      "physicalID": 24
    },
    {
      "name": "XPHY25",
      "type": 2,
      "physicalID": 25
    },
    {
      "name": "XPHY26",
      "type": 2,
      "physicalID": 26
    },
    {
      "name": "XPHY27",
      "type": 2,
      "physicalID": 27
    },
    {
      "name": "XPHY28",
      "type": 2,
      "physicalID": 28
    },
    {
      "name": "XPHY29",
      "type": 2,
      "physicalID": 29
    },
    {
      "name": "XPHY30",
      "type": 2,
      "physicalID": 30
    },
    {
      "name": "XPHY31",
      "type": 2,
      "physicalID": 31
    },
    {
      "name": "XPHY32",
      "type": 2,
      "physicalID": 32
    },
    {
      "name": "XPHY33",
      "type": 2,
      "physicalID": 33
    },
    {
      "name": "XPHY34",
      "type": 2,
      "physicalID": 34
    },
    {
      "name": "XPHY35",
      "type": 2,
      "physicalID": 35
    },
    {
      "name": "XPHY36",
      "type": 2,
      "physicalID": 36
    },
    {
      "name": "XPHY37",
      "type": 2,
      "physicalID": 37
    },
    {
      "name": "XPHY38",
      "type": 2,
      "physicalID": 38
    },
    {
      "name": "XPHY39",
      "type": 2,
      "physicalID": 39
    },
    {
      "name": "XPHY40",
      "type": 2,
      "physicalID": 40
    },
    {
      "name": "XPHY41",
      "type": 2,
      "physicalID": 41
    },
    {
      "name": "XPHY42",
      "type": 2,
      "physicalID": 42
    },
    {
      "name": "XPHY43",
      "type": 2,
      "physicalID": 43
    },
    {
      "name": "XPHY44",
      "type": 2,
      "physicalID": 44
    },
    {
      "name": "XPHY45",
      "type": 2,
      "physicalID": 45
    },
    {
      "name": "XPHY46",
      "type": 2,
      "physicalID": 46
    },
    {
      "name": "XPHY47",
      "type": 2,
      "physicalID": 47
    },
    {
      "name": "XPHY48",
      "type": 2,
      "physicalID": 48
    },
    {
      "name": "XPHY49",
      "type": 2,
      "physicalID": 49
    },
    {
      "name": "XPHY50",
      "type": 2,
      "physicalID": 50
    },
    {
      "name": "XPHY51",
      "type": 2,
      "physicalID": 51
    },
    {
      "name": "XPHY52",
      "type": 2,
      "physicalID": 52
    },
    {
      "name": "XPHY53",
      "type": 2,
      "physicalID": 53
    },
    {
      "name": "XPHY54",
      "type": 2,
      "physicalID": 54
    },
    {
      "name": "XPHY55",
      "type": 2,
      "physicalID": 55
    },
    {
      "name": "XPHY56",
      "type": 2,
      "physicalID": 56
    },
    {
      "name": "XPHY57",
      "type": 2,
      "physicalID": 57
    },
    {
      "name": "XPHY58",
      "type": 2,
      "physicalID": 58
    },
    {
      "name": "XPHY59",
      "type": 2,
      "physicalID": 59
    },
    {
      "name": "XPHY60",
      "type": 2,
      "physicalID": 60
    },
    {
      "name": "XPHY61",
      "type": 2,
      "physicalID": 61
    },
    {
      "name": "XPHY62",
      "type": 2,
      "physicalID": 62
    },
    {
      "name": "XPHY63",
      "type": 2,
      "physicalID": 63
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 23
      },
      "profile": {
        "speed": 100000,
        "iphy": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528,
          "medium": 2,
          "interfaceMode": 3,
          "interfaceType": 3
        },
        "xphyLine": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528
        },
        "xphySystem": {
          "numLanes": 4,
          "modulation": 1,
          "fec": 528
        }
      }
    },
    {
      "factor": {
        "profileID": 25
      },
      "profile": {
        "speed": 200000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceMode": 3,
          "interfaceType": 3
        },
        "xphyLine": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 544
        },
        "xphySystem": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 544
        }
      }
    },
    {
      "factor": {
        "profileID": 26
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 11,
          "medium": 2,
          "interfaceMode": 4,
          "interfaceType": 4
        },
        "xphyLine": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 544
        },
        "xphySystem": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 544
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
SandiaPlatformMapping::SandiaPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}
} // namespace fboss
} // namespace facebook
